namespace NetworkManager {
    public class SettingsHelper {
    public signal void bus_disappeared ();
    public signal void bus_appeared ();

    public SettingsHelper () {
      updated_connections = new HashTable <string, GLib.Variant> (str_hash, direct_equal);
    }

    public bool in_bus () {
      return _in_bus;
    }

    private void bus_name_appeared_cb (DBusConnection con, string name, string owner) {
      return;
    }

    public string? get_connection_path_by_uid (string uuid) {
      return conn_path;
    }

    public void update_connection (string path, GLib.Variant properties) throws IOError {
      return;
    }

    public string? add_connection (GLib.Variant conn) throws IOError {
      return null;
    }

    /* Mock entry points */
    private bool _in_bus = false;
    private string? conn_path = null;

    private HashTable <string, GLib.Variant> updated_connections;

    public void emit_bus_appeared () {
      _in_bus = true;
      bus_appeared ();
    }

    public void emit_bus_disappeared () {
      _in_bus = false;
      bus_disappeared ();
    }
  }
}
