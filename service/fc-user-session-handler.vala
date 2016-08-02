namespace FleetCommander {
  public class UserSessionHandler {
    private UserIndex       index;
    private Logind.Manager? logind = null;
    private List<ConfigurationAdapter> handlers = null;
    private CacheData profiles;

    public UserSessionHandler (UserIndex index, CacheData profiles) {
      this.index = index;
      this.profiles = profiles;

      Bus.watch_name (BusType.SYSTEM,
                      "org.freedesktop.login1",
                      BusNameWatcherFlags.AUTO_START,
                      bus_name_appeared_cb,
                      (con, name) => {
                        warning ("org.freedesktop.login1 bus name vanished");
                        logind = null;
                      });
      index.updated.connect (bootstrap);
    }

    public void register_handler (ConfigurationAdapter handler) {
      if (handler is ConfigurationAdapterNM) {
        (handler as ConfigurationAdapterNM).bus_appeared.connect ((nma) => {
          Logind.User[] users;
          try {
            users = logind.list_users ();
          } catch (Error e) {
            warning ("There was an error listing logged in users using the logind dbus interface: %s", e.message);
            return;
          }

          nma.bootstrap (index, profiles, users);
        });
      }
      handlers.append (handler);
    }

    private void bootstrap () {
      if (logind == null) {
        warning ("org.freedesktop.logind1 bus name is not present");
        return;
      }

      Logind.User[] users;
      try {
        users = logind.list_users ();
      } catch (Error e) {
        warning ("There was an error listing logged in users using the logind dbus interface: %s", e.message);
        return;
      }

      foreach (var handler in handlers) {
        handler.bootstrap (index, profiles, users);
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
      bootstrap ();
    }

    private void user_logged_cb (uint32 user_id, ObjectPath path) {
      debug ("User logged in with uid: %u", user_id);
      foreach (var handler in handlers) {
        handler.update (index, profiles, user_id);
      }
      index.flush ();
    }
  }
}
