namespace FleetCommander {
  public class UserSessionHandler {
    private UserIndex       index;
    private Logind.Manager? logind;
    private List<string>    merged_profiles = null;
    private string          dconf_db_path;
    private string          dconf_profile_path;

    public UserSessionHandler (UserIndex index,
                               string    dconf_db_path,
                               string    dconf_profile_path) {
      this.index = index;
      this.dconf_db_path      = dconf_db_path;
      this.dconf_profile_path = dconf_profile_path;
      logind = null;

      Bus.watch_name (BusType.SYSTEM,
                      "org.freedesktop.login1",
                      BusNameWatcherFlags.AUTO_START,
                      bus_name_appeared_cb,
                      (con, name) => {
                        warning ("org.freedesktop.login1 bus name vanished");
                        logind = null;
                      });
      index.updated.connect (update_dconf_profiles);
    }

     private void update_dconf_profiles () {
      if (logind == null)
        return;

      if (dconf_db_can_write (dconf_db_path) == false)
        return;
      try {
        foreach (var user in logind.list_users()) {
          create_dconf_profile_for_user (user.uid);
        }
      } catch (IOError e) {
        warning ("There was an error calling ListUsers in /org/freedesktop/login1");
      }

      index.flush ();
    }

    private void bus_name_appeared_cb (DBusConnection con, string name, string owner) {
      debug("org.freedesktop.login1 bus name appeared");
      try {
        logind = Bus.get_proxy_sync (BusType.SYSTEM,
                                     "org.freedesktop.login1",
                                     "/org/freedesktop/login1");
        logind.user_new.connect (user_logged_cb);
      } catch (Error e) {
        debug ("There was an error trying to connect to /org/freedesktop/logind1 in the system bus: %s", e.message);
      }
      update_dconf_profiles ();
    }

    private void user_logged_cb (uint32 user_id, ObjectPath path) {
      if (dconf_db_can_write (dconf_db_path) == false)
        return;

      debug ("User logged in with uid: %u", user_id);
      create_dconf_profile_for_user (user_id);
      index.flush ();
    }

    private void create_dconf_profile_for_user (uint32 uid) {
      var dconf_profile_data = "user-db:user";

      var groups = Posix.get_groups_for_user (uid);
      var user_index  = index.get_user_profiles ();
      var group_index = index.get_group_profiles ();
      var name = Posix.uid_to_user_name (uid);

      if (name == null) {
        warning ("%u user has no user name string", uid);
        return;
      }

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
        delete_dconf_profile (name);
        merged_profiles = null;
        return;
      }

      foreach (var profile in merged_profiles) {
        dconf_profile_data += "\nsystem-db:%s".printf("fleet-commander-" + profile);
      }
      merged_profiles = null;

      write_dconf_profile (name, dconf_profile_data);
    }

    private void delete_dconf_profile (string name) {
      var path = string.join("/", dconf_profile_path, "u:" + name);
      try {
        var obsolete_profile = File.new_for_path (path);
        if (obsolete_profile.query_exists() == true)
          obsolete_profile.delete ();
      } catch (Error e) {
        warning ("could not remove unused profile %s", path);
      }
    }

    private void write_dconf_profile (string user_name, string data) {
      var path = string.join("/", dconf_profile_path, "u:" + user_name);
      debug ("Attempting to write dconf profile in %s", path);
      try {
        var dconf_profile = File.new_for_path (path);
        var w = dconf_profile.replace(null, true, FileCreateFlags.NONE, null);
        var d = new DataOutputStream(w);
        d.put_string(data);
      } catch (Error e) {
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

      if (merged_profiles.find_custom (profile, GLib.strcmp) == null) {
        merged_profiles.prepend (profile);
      }
    }
  }
}
