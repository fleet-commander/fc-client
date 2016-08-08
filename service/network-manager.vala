namespace NetworkManager {
  [DBus (name = "org.freedesktop.NetworkManager.Settings")]
  public interface Settings : GLib.Object {
    public abstract string GetConnectionByUuid (string uuid) throws IOError;
    public abstract string AddConnection ([DBus (signature = "(a{sa{sv}})")] GLib.Variant connection) throws IOError;
  }

  [DBus (name = "org.freedesktop.NetworkManager.Settings.Connection")]
  public interface Connection : GLib.Object {
    //public abstract GLib.Variant GetSettings () throws IOError;
    public abstract void Update ([DBus (signature = "(a{sa{sv}})")] GLib.Variant properties) throws IOError;
    public abstract void Save () throws IOError;
  }

  public class SettingsHelper {
    private Settings? settp = null;
    private Connection? connp = null;
    public signal void bus_dissapeared ();
    public signal void bus_appeared ();
    public SettingsHelper () {
      debug ("Watching for bus name org.freedesktop.NetworkManager in the system bus");
      Bus.watch_name (BusType.SYSTEM,
                      "org.freedesktop.NetworkManager",
                      BusNameWatcherFlags.AUTO_START,
                      bus_name_appeared_cb,
                      (con, name) => {
                        warning ("org.freedesktop.NetworkManager bus name disappeared");
                        settp = null;
                        connp = null;
                        bus_disappeared ();
                      });
    }

    public bool in_bus () {
      return settp != null;
    }

    private void bus_name_appeared_cb (DBusConnection con, string name, string owner) {
      debug("org.freedesktop.NetworkManager bus name appeared");
      try {
         settp = Bus.get_proxy_sync<NetworkManager.Settings> (BusType.SYSTEM,
                                     "org.freedesktop.NetworkManager",
                                     "/org/freedesktop/NetworkManager/Settings");
         bus_appeared ();
      } catch (Error e) {
        warning ("There was an error trying to create the dbus proxy: %s", e.message);
      }
    }

    public string? get_connection_path_by_uid (string uuid) {
      string? result = null;
      try {
        result = settp.GetConnectionByUuid (uuid);
      } catch (Error e) {
        //FIXME: discriminate non existing connection from other errors, assume not existing for now
        debug ("There was no NetworkManager connection with uuid %s or there was an error doing the DBus call:\n%s",
               uuid, e.message);
      }

      return result;
    }

    public void update_connection (string path, GLib.Variant properties) throws IOError {
       var conn = Bus.get_proxy_sync<NetworkManager.Connection> (BusType.SYSTEM,
                                     "org.freedesktop.NetworkManager",
                                     path);
       conn.Update (properties);
    }

    public string? add_connection (GLib.Variant conn) throws IOError {
      return settp.AddConnection (conn);
    }
  }
}
