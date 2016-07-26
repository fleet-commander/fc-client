namespace FleetCommander {
  public class ConfigurationAdapterDconfProfiles : ConfigurationAdapter, Object {
    private string          dconf_profile_path;
    private List<string>    merged_profiles = null;

    public ConfigurationAdapterDconfProfiles (string dconf_profile_path) {
      this.dconf_profile_path = dconf_profile_path;
    }

    private void bootstrap (UserIndex index, Logind.User[] users) {
      foreach (var user in users) {
        update (index, user.uid);
      }
    }

    public void update (UserIndex index, uint32 uid) {
      if (dconf_db_can_write (dconf_profile_path) == false) {
        warning ("There was an error trying to write to %s", dconf_profile_path);
        return;
      }
 
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

      write_dconf_profile (uid, dconf_profile_data);
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

    private void write_dconf_profile (uint32 uid, string data) {
      var path = dconf_profile_path + "/%u".printf(uid);
      var dconf_profile = File.new_for_path (path);

      // Create parent directory for profile if it doesn't exists
      if (dconf_profile.get_parent ().query_exists () == false) {
        try {
          dconf_profile.get_parent ().make_directory_with_parents ();
        } catch (Error e) {
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
