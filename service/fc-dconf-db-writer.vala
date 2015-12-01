namespace FleetCommander {
  internal class DconfDbWriter {
    private CacheData cache;
    private string    dconf_db_path;
    private string[]  VALIDATED_KEYS = {"uid", "settings"};
    private string?   uid;
    private KeyFile?   keyfile;

    internal DconfDbWriter(CacheData cache,
                           string    dconf_db_path) {
      this.cache = cache;
      this.dconf_db_path = dconf_db_path;
      this.cache.parsed.connect(update_databases);
    }

    internal void update_databases() {
      debug("Updating dconf databases");
      var root = cache.get_root();
      if (root == null)
        return;

      if (dconf_db_can_write(dconf_db_path) == false)
        return;

      remove_current_profiles ();

      root.get_array().foreach_element ((a, i, n) => {
        //FIXME: Use etag to check if we need to replace this profile
        var profile = n.get_object();
        if (profile == null) {
          warning("Cached profile is not a JSON object (element #%u)", i);
          return;
        }

        if (object_has_members (profile, VALIDATED_KEYS) == false) {
          warning ("Could not find mandatory keys in cached profile #%u", i);
          return;
        }

        commit_profile(profile);
      });

      call_dconf_update();
    }

    private void remove_current_profiles () {
      Dir db_path;
      string[] current_profiles = {};

      try {
        db_path = Dir.open (dconf_db_path);
      } catch (Error e) {
        warning ("Could not open dconf database path %s", dconf_db_path);
        return;
      }

      var root = cache.get_root ();
      if (root != null && root.get_array () != null) {
        root.get_array ().foreach_element ((a, i, n) => {
          var profile = n.get_object ();
          if (profile == null)
            return;
          if (profile.has_member ("uid") == false)
            return;
          current_profiles += profile.get_string_member ("uid");
        });
      }

      for (var db_child = db_path.read_name (); db_child != null; db_child = db_path.read_name ()) {
        bool exists = false;

        if (db_child.has_prefix ("fleet-commander-") == false)
           continue;

        var path = string.join ("/", dconf_db_path, db_child);

        if (db_child.has_suffix (".d") && FileUtils.test (path, FileTest.IS_DIR)) {
          foreach (var existing in current_profiles) {
            if (path.has_suffix (existing + ".d") == false)
              continue;
            exists = true;
            break;
          }
          if (exists)
            continue;

          var keyfile  = string.join("/", path,  "generated");
          var locks    = string.join("/", path,  "locks");
          var lockfile = string.join("/", locks, "generated");
          FileUtils.remove (keyfile);
          FileUtils.remove (lockfile);
          FileUtils.remove (locks);
          FileUtils.remove (path);
          continue;
        }

        if (FileUtils.test (path, FileTest.IS_REGULAR) == false)
          continue;

        /* Find out if this profile is still valid */
        foreach (var existing in current_profiles) {
          if (path.has_suffix (existing) == false)
            continue;
          exists = true;
          break;
        }
        if (exists)
          continue;

        debug ("Removing profile database %s", path);
        if (FileUtils.remove(path) == -1) {
          warning ("There was an error attempting to remove %s", path);
        }
      }
    }

    private void call_dconf_update () {
      Subprocess dconf_update;
      try {
        dconf_update = new Subprocess.newv ({"dconf", "update"}, SubprocessFlags.NONE);
        dconf_update.wait ();
      } catch (Error e) {
        warning ("Could not call dconf update, %s", e.message);
        return;
      }

      if (dconf_update.get_if_exited () == false) {
        warning ("dconf update process did not exit");
        dconf_update.force_exit ();
        return;
      }

      if (dconf_update.get_exit_status () != 0) {
        warning ("dconf update failed");
      }
    }

    /* This function makes sure that we can write the keyfile for a profile */
    internal bool check_filesystem_for_profile (File settings_keyfile) {
      debug("Checking whether we can write profile data to %s", settings_keyfile.get_path ());

      if (settings_keyfile.query_exists() == true) {
        FileInfo info;
        try {
          info = settings_keyfile.query_info("access::can-delete", FileQueryInfoFlags.NONE);
        } catch (Error e) {
          warning ("There was an error querying %s attributes", settings_keyfile.get_path ());
          return false;
        }
        if (info.get_attribute_boolean("access::can-delete") == false) {
          warning ("Could not replace file %s", settings_keyfile.get_path ());
          return false;
        }
      }

      var basedir = settings_keyfile.get_parent().get_path();
      if (FileUtils.test (basedir, FileTest.EXISTS) == false) {
        if (DirUtils.create_with_parents (basedir, 0755) == -1) {
          warning ("Could not create directory %s", basedir);
          return false;
        }
      }
      if (FileUtils.test (basedir, FileTest.IS_DIR) == false) {
        warning ("%s exists but is not a file", basedir);
        return false;
      }

      if (Posix.access(basedir, Posix.W_OK | Posix.X_OK) != 0) {
        warning ("Cannot write data onto %s", basedir);
        return false;
      }

      return true;
    }

    internal void commit_profile (Json.Object profile) {
      debug("Committing profile");
      Json.Object?      settings;
      File              generated;

      if ((uid = profile.get_member("uid").get_string()) == null) {
        warning ("profile uid JSON member was not a string");
        return;
      }

      /* We make sure we can write this profile and go ahead*/
      generated = File.new_for_path (string.join("/", dconf_db_path, "fleet-commander-" + uid + ".d",
                                                 "generated"));
      if (check_filesystem_for_profile (generated) == false)
        return;

      /* We make sure the JSON object is sane */
      if ((settings = profile.get_member("settings").get_object()) == null) {
        warning ("profile %s: 'settings' JSON member is not a JSON object", uid);
        return;
      }


      keyfile = new KeyFile ();
      var gsettings = get_changes_array (settings, uid, "org.gnome.gsettings");
      var libreoffice = get_changes_array (settings, uid, "org.libreoffice.registry");

      if (gsettings != null)
        gsettings.foreach_element(add_change_to_keyfile);
      if (libreoffice != null)
        libreoffice.foreach_element(add_change_to_keyfile);

      debug("Saving profile keyfile in %s", generated.get_path());
      try {
        keyfile.save_to_file (generated.get_path ());
      } catch (Error e) {
        warning ("There was an error saving profile settings in %s", generated.get_path ());
      }

      keyfile = null;
      uid = null;
    }

    private Json.Array? get_changes_array (Json.Object settings, string uid, string collector) {
      Json.Array? changes = null;

      var node = settings.get_member(collector);
      if (node == null)
        debug("profile %s: does not have %s section", uid, collector);
      if ((changes = node.get_array()) == null)
        warning ("profile %s: %s member of settings is not a JSON array", uid, collector);

      return changes;
    }

    private void add_change_to_keyfile (Json.Array a, uint i, Json.Node n) {
      var change = n.get_object();
      if (change == null) {
        warning ("profile %s: gsettings change item #%u should be a JSON object", uid, i);
        return;
      }

      if (object_has_members (change, {"key", "value"}) == false) {
        warning ("profile %s: Could not find mandatory keys in change %u", uid, i);
        return;
      }

      var key = change.get_member("key").get_string();
      if (key == null) {
        warning ("profile %s: 'key' member of change %u is not a string", uid, i);
      }

      string? signature = null;
      var keyname  = GLib.Path.get_basename (key);
      var path     = GLib.Path.get_dirname  (key);
      path = path.slice (1, path.length);

      /* TODO: get the logger to send the signature */
      if (change.has_member ("signature"))
        signature = change.get_string_member ("signature");
      else
        warning ("profile %s: could not find signature for key %s", uid, key);

      Variant? variant;
      try {
        variant = Json.gvariant_deserialize(change.get_member("value"), signature);
      } catch (Error e) {
        warning ("profile %s: could not deserialize JSON to GVariant for key %u", key, i);
        return;
      }

      keyfile.set_string(path, keyname, variant.print(true));
    }
  }
}
