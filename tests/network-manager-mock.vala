namespace NetworkManager {
  public class SettingsHelper {
    public signal void bus_disappeared ();
    public signal void bus_appeared ();

    private const string OBJPATHPREFIX = "/org/freedesktop/NetworkManager/Settings/";

    public SettingsHelper () {
      updated_connections = new HashTable <string, GLib.Variant> (str_hash, direct_equal);
      added_connections = new HashTable <string, GLib.Variant> (str_hash, direct_equal);
    }

    public bool in_bus () {
      return _in_bus;
    }

    private void bus_name_appeared_cb (DBusConnection con, string name, string owner) {
      return;
    }

    public string? get_connection_path_by_uid (string uuid) {
      if (added_connections.lookup (uuid) != null)
        return OBJPATHPREFIX + uuid;
      foreach (var present in present_connections) {
        if (present == uuid)
          return  OBJPATHPREFIX + uuid;
      }
      return null;
    }

    public void update_connection (string path, GLib.Variant properties) throws IOError {
      updated_connections.insert (path, properties);
    }

    public string? add_connection (GLib.Variant conn) throws IOError {
      string uuid = conn.lookup_value ("connection", null).lookup_value ("uuid", null).get_string ();
      added_connections.insert (uuid, conn);
      return OBJPATHPREFIX + uuid;
    }

    /* Mock entry points */
    private bool _in_bus = false;
    private string? conn_path = null;
    private string[] present_connections;

    public HashTable <string, GLib.Variant> updated_connections;
    public HashTable <string, GLib.Variant> added_connections;

    public void emit_bus_appeared () {
      _in_bus = true;
      bus_appeared ();
    }

    public void emit_bus_disappeared () {
      _in_bus = false;
      bus_disappeared ();
    }

    public void add_present_uuid (string uuid) {
      present_connections += uuid;
    }
  }
}
