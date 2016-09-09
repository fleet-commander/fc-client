namespace FleetCommander {
  public class ConfigurationAdapterDconfProfiles : ConfigurationAdapter, Object {
    private string          dconf_profile_path;
    private List<string>    merged_profiles = null;
    private bool            in_bootstrap = false;
    private string[]?       dconf_data_profiles = null;

    public ConfigurationAdapterDconfProfiles (string dconf_profile_path) {
      this.dconf_profile_path = dconf_profile_path;
    }

    private string[]? get_dconf_data_profiles (CacheData profiles_cache) {
      debug ("Finding profiles that contain dconf data");
      string[] result = {};

      var profiles = profiles_cache.get_root () != null ? profiles_cache.get_root ().get_array () : null;
      if (profiles == null) {
        debug ("profiles.json does not contain a root array element");
        return null;
      }

      profiles.foreach_element ((a, i, n) => {
        Json.Node uid_node;
        try {
          uid_node = Json.Path.query ("$.uid", n);
        } catch (GLib.Error e) {
          warning ("There was an error querying for data in profile %u in profiles.json", i);
          return;
        }

        if (uid_node.get_array().get_length () < 1) {
          warning ("Profile %u in profiles.json does not have uid", i);
          return;
        }

        var uid = uid_node.get_array().get_element(0).get_string ();
        if (uid == null) {
          warning ("profile %u did not have uid string", i);
          return;
        }

        string[] config_domains = {"$['settings']['org.gnome.gsettings']", "$['settings']['org.libreoffice.registry']"};
        foreach (var domain in config_domains) {
          Json.Node matches;
          try {
            matches = Json.Path.query (domain, n);
          } catch (GLib.Error e) {
            warning ("There was an error trying to compile JSON Path expression %s : %s", domain, e.message);
            continue;
          }

          var matches_array = matches.get_array ();
          if (matches_array != null && matches_array.get_length () < 1)
            continue;

          var settings = matches_array.get_array_element (0);
          if (settings != null && settings.get_length () < 1)
            continue;

          result += uid;
        }
      });

      return result;
    }

    public void bootstrap (UserIndex index, CacheData profiles_cache, Logind.User[] users) {
      in_bootstrap = true;
      dconf_data_profiles = get_dconf_data_profiles (profiles_cache);
      foreach (var user in users) {
        update (index, profiles_cache, user.uid);
      }
      in_bootstrap = false;
      dconf_data_profiles = null;
    }

    public void update (UserIndex index, CacheData profiles_cache, uint32 uid) {
      debug ("Update dconf profile for user %u", uid);
      if (FileUtils.test (dconf_profile_path, FileTest.EXISTS) == false) {
        if (DirUtils.create_with_parents (dconf_profile_path, 0755) != 0) {
          warning ("Could not create directory %s", dconf_profile_path);
          return;
        }
      }

      if (FileUtils.test ("%s/%u".printf (dconf_profile_path, uid), FileTest.EXISTS)) {
        if (Posix.access(dconf_profile_path, Posix.W_OK | Posix.X_OK) != 0) {
          warning ("Cannot write data onto %s", dconf_profile_path);
          return;
        }
      }

      var name = Posix.uid_to_user_name (uid);

      if (in_bootstrap == false)
        dconf_data_profiles = get_dconf_data_profiles (profiles_cache);

      if (name == null) {
        warning ("%u user has no user name string", uid);
        dconf_data_profiles = null;
        return;
      }

      if (dconf_data_profiles.length < 1) {
        debug ("There are no profiles with dconf data");
        delete_dconf_profile (uid);
        return;
      }

      var dconf_profile_data = "user-db:user";

      var groups = Posix.get_groups_for_user (uid);
      var user_index  = index.get_user_profiles ();
      var group_index = index.get_group_profiles ();

      merged_profiles = null;

      if (user_index != null         &&
          user_index.has_member (name)) {
        var profiles = user_index.get_array_member (name);
        if (profiles != null) {
          profiles.foreach_element (add_elements_to_merged_profiles);
        }
      }

      /* For each group that the user belongs to we check
       * if there is are profiles assigned to it. */
      if (group_index != null      &&
          group_index.get_size () > 0) {
        foreach (var group in groups) {
          if (group_index.has_member(group) == false)
            continue;

          var profiles = group_index.get_array_member (group);
          if (profiles == null)
            continue;
          profiles.foreach_element (add_elements_to_merged_profiles);
        }
      }

      if (merged_profiles.length() < 1) {
        delete_dconf_profile (uid);
        merged_profiles = null;
        dconf_profile_data = null;
        return;
      }

      foreach (var profile in merged_profiles) {
        dconf_profile_data += "\nsystem-db:%s".printf("fleet-commander-" + profile);
      }

      write_dconf_profile (uid, dconf_profile_data);

      merged_profiles = null;
      dconf_profile_data = null;
    }

    private void delete_dconf_profile (uint32 uid) {
      var path = "%s/%u".printf(dconf_profile_path, uid);
      try {
        var obsolete_profile = File.new_for_path (path);
        if (obsolete_profile.query_exists() == true)
          obsolete_profile.delete ();
      } catch (Error e) {
        warning ("could not remove unused dconf profile %s", path);
      }
    }

    private void write_dconf_profile (uint32 uid, string data) {
      var path = dconf_profile_path + "/%u".printf(uid);
      var dconf_profile = File.new_for_path (path);

      // Create parent directory for profile if it doesn't exists
      if (dconf_profile.get_parent ().query_exists () == false) {
        try {
          dconf_profile.get_parent ().make_directory_with_parents ();
        } catch (GLib.Error e) {
          warning ("Could not create directory %s: %s",
                   dconf_profile.get_parent ().get_path (), e.message);
          return;
        }
      }

      debug ("Attempting to write dconf profile in %s", path);
      try {
        var w = dconf_profile.replace(null, false, FileCreateFlags.NONE, null);
        var d = new DataOutputStream(w);
        d.put_string(data);
      } catch (GLib.Error e) {
        warning("Could not rewrite %s: %s", path, e.message);
        return;
      }
    }

    private void add_elements_to_merged_profiles (Json.Array a, uint i, Json.Node n) {
      var profile = n.get_string();
      if (profile == null) {
        warning ("user -> profiles index contained an element that is not a string");
        return;
      }

      // Avoid duplicates
      foreach (var p in merged_profiles) {
        if (p != profile)
          continue;

        debug ("%s is already listed as a dconf profile", profile);
        return;
      }

      //Filter by profiles with dconf data
      foreach (var valid_profile in dconf_data_profiles) {
        if (profile != valid_profile)
          continue;

        merged_profiles.prepend (profile);
        return;
      }
      debug ("%s did not hold any dconf data", profile);
    }
  }
}
