namespace FleetCommander {
  public class CacheData {
    private Json.Node?     root = null;
    private File           profiles;
    private ContentMonitor monitor;

    public signal void parsed();

    public CacheData (string path) {
      profiles = File.new_for_path (path);
      monitor = new ContentMonitor (profiles.get_path ());
      monitor.content_updated.connect (() => {
        parse ();
      });
      parse ();
    }

    public string get_path () {
      return profiles.get_path();
    }

    public Json.Node? get_root() {
      return root;
    }

    private void parse () {
      if (profiles.query_exists () == false) {
        debug ("cache file %s: does not exist", profiles.get_path ());
        root = null;
        parsed ();
        return;
      }

      debug ("Parsing cache file: %s", profiles.get_path());
      var parser = new Json.Parser();
      try {
        parser.load_from_stream (profiles.read());
      } catch (Error e) {
        warning ("There was an error parsing %s: %s", profiles.get_path(), e.message);
        return;
      }

      root = parser.get_root();
      if (root == null) {
        warning ("Root JSON element of profile cache is empty");
      } else if (root.get_array() == null) {
        warning ("Root JSON element of profile cache is not an array");
        root = null;
      }
      parsed();
    }
  }
}
