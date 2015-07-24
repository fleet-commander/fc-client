namespace FleetCommander {
  public class CacheData {
    private File        profiles;
    private FileMonitor monitor;
    private Json.Node?  root = null;
    private bool        timeout;
    private uint        cookie = 0;

    public signal void parsed();

    public CacheData (string cache_path) {
      profiles = File.new_for_path(cache_path + "/profiles.json");
      monitor = profiles.monitor_file(FileMonitorFlags.NONE);
      monitor.changed.connect((file, other_file, event) => {
        switch (event) {
          case FileMonitorEvent.CREATED:
          case FileMonitorEvent.CHANGED:
            debug("%s was changed or created", profiles.get_path());
            break;
          case FileMonitorEvent.DELETED:
            debug("%s was deleted", profiles.get_path());
            break;
          default:
            debug("Unhandled FileMonitor event");
            return;
        }
        parse_async(true);
      });

      parse();
    }
    
    public string get_path () {
      return profiles.get_path();
    }
    
    public Json.Node? get_root() {
      return root;
    }

    private void parse_async (bool timeout) {
      this.timeout = timeout;
      if (cookie != 0) {
        debug("A timeout to parse %s is already installed", profiles.get_path());
        return;
      }

      cookie = Timeout.add (1000, () => {
        if (this.timeout == true) {
          debug("Changes just happened, waiting another second without changes");
          this.timeout = false;
          return true;
        }

        parse();

        timeout = false;
        cookie  = 0;
        return false;
      });
    }

    private void parse () {
      debug ("Parsing cache file: %s", profiles.get_path());
      var parser = new Json.Parser();
      try {
        parser.load_from_stream (profiles.read());
      } catch (Error e) {
        warning("There was an error parsing %s: %s", profiles.get_path(), e.message);
        return;
      }

      root = parser.get_root();
      if (root == null) {
        warning ("Root JSON element of profile cache is empty");
      } else if (root.get_array() == null) {
        warning ("Root JSON element of profile cache is not an array");
        root = null;
      } else {
        parsed();
      }
    }
  }
}
