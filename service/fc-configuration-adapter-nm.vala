namespace FleetCommander {
  public class ConfigurationAdapterNM : ConfigurationAdapter, Object {
    NetworkManager.Settings? nms = null;

    public signal void bus_appeared (ConfigurationAdapterNM nma);
    public signal void bus_disappeared (ConfigurationAdapterNM nma);

    public ConfigurationAdapterNM () {
      Bus.watch_name (BusType.SYSTEM,
                      "org.freedesktop.NetworkManager",
                      BusNameWatcherFlags.AUTO_START,
                      bus_name_appeared_cb,
                      (con, name) => {
                        warning ("org.freedesktop.NetworkManager bus name disappeared");
                        nms = null;
                        bus_disappeared (this);
                      });
    }

    private void bus_name_appeared_cb (DBusConnection con, string name, string owner) {
      debug("org.freedesktop.NetworkManager bus name appeared");
      try {
         nms = Bus.get_proxy_sync<NetworkManager.Settings> (BusType.SYSTEM,
                                     "org.freedesktop.NetworkManager",
                                     "/org/freedesktop/NetworkManager/Settings");

         //connection = nms.GetConnectionByUuid ("5d4bfafe-0da0-42de-b425-e7b4c2f03ddc");
      } catch (Error e) {
        debug ("There was an error trying to create the dbus proxy: %s", e.message);
      }
      bus_appeared (this);
    }

    public void bootstrap (UserIndex index, CacheData profiles_cache, Logind.User[] users) {
      debug ("Bootstrapping NetworkManager settings for all logged in users");
      if (nms == null) {
        debug ("NetworkManager's bus name was not connected");
        return;
      }

      foreach (var user in users) {
        update (index, profiles_cache, user.uid);
      }
    }

    public void update (UserIndex index, CacheData profiles_cache, uint32 uid) {
      debug ("Updating NM connections for user %u", uid);
      if (nms == null) {
        debug ("NetworkManager's bus name was not connected");
        return;
      }

      var name = Posix.uid_to_user_name (uid);
      var groups = Posix.get_groups_for_user (uid);

      var profile_uids = index.get_profiles_for_user_and_groups (name, groups);

      if (profile_uids.length < 1) {
        debug ("User %u did not have any profiles assigned", uid);
        return;
      }

      var profiles = profiles_cache.get_profiles (profile_uids);

      if (profiles == null || profiles.length != profile_uids.length) {
        string uids = "", found = "";
        foreach (var u in profile_uids) {
          uids = "%s, %s".printf (uids, u);
        }

        if (profiles == null) {
          debug ("No profiles found from list: %s", uids);
          return;
        }

        //FIXME: only iterate if debugging is enabled;
        debug ("Some profiles applied to user %u were not found", uid);
      }

      foreach (var profile in profiles) {
        var gen = new Json.Generator ();
        var node = new Json.Node (Json.NodeType.OBJECT);
        node.set_object (profile);
        gen.set_root (node);
        
        var settings = profile.get_object_member("settings");
        var puuid = profile.get_string_member ("uid");
        if (settings == null)
          continue;

        var nm = settings.get_array_member ("org.freedesktop.NetworkManager");
        if (nm == null)
          continue;

        nm.foreach_element ((a, i, connection_node) => {
          var root = connection_node.get_object ();
/*          if (root == null)
            return;

          var json_obj = root.get_object_member ("json");
          if (json_obj == null) {
              warning ("NetworkManager connection %u from profile %s had no 'json' member and will be ignored", i, puuid);
              return;
          }

          var conn = root.get_object_member ("connection");
          if (conn == null) {
            warning ("NetworkManager connection %u from profile %s does not have json.connection.uuid as string and will be ignored",
                     i, puuid);
          }

          var uuid = json_obj.get_string_member ("uuid");
          if (uuid == null) {
            warning ("NetworkManager connection %u from profile %s does not have json.connection.uuid as string and will be ignored",
                     i, puuid);
            return;
          }
*/
          var uuid = Json.Path.query ("$.json.connection.uuid", connection_node).get_array ().get_string_element (0);
          if (uuid == null) {
            warning ("NetworkManager connection %u from profile %s does not have a json.connection.uuid in the form of a string",
                     i, puuid);
            return;
          }

          string? nm_conn_path = null;
          try {
            nm_conn_path = nms.GetConnectionByUuid (uuid);
          } catch (GLib.Error e) {
            //FIXME: discriminate non existing connection from other errors, assume not existing for now
            debug ("There was no NetworkManager connection with uuid %s or there was an error doing the DBus call:\n%s",
                   uuid, e.message);
          }

          var encoded_data = root.get_string_member ("data");
          if (encoded_data == null) {
            warning ("NetworkManager connection %u from profile %s does not have a 'data' member",
                     i, puuid);
            return;
          }

          //FIXME: Add permissions and other modifications here

          var decoded = new Bytes (Base64.decode (encoded_data));
          var conn_variant = new Variant.from_bytes (new VariantType("a{sa{sv}}"), decoded, false);
          //FIXME: swap bytes if big endian

          try {
            if (nm_conn_path == null) {
              nms.AddConnection (conn_variant);
            } else {
              update_connection (nm_conn_path, conn_variant);
            }
          } catch (GLib.Error e) {
            warning ("Could not add/update %u connection from profile %s, there was an error making the DBus call:\n%s",
                     i, puuid, e.message);
          }
        });
      }
    }

    private void update_connection (string path, GLib.Variant properties) throws Error {
       var conn = Bus.get_proxy_sync<NetworkManager.Connection> (BusType.SYSTEM,
                                     "org.freedesktop.NetworkManager",
                                     path);
       conn.Update (properties);
    }
  }
}
