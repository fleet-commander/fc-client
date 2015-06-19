using Soup;
using Json;

/*
namespace Logind {
  internal struct User {
    uint32 uid;
    string name;
    string path;
  }

  [DBus (name = "org.freedesktop.login1.Manager")]
  interface Manager : GLib.Object {
    public abstract User[] list_users () throws IOError;
    public abstract signal void user_new     (uint32 uid, ObjectPath path);
    public abstract signal void user_removed (uint32 uid, ObjectPath path);
  }
}
*/

namespace FleetCommander {
  internal ConfigReader config;

  internal class Cache {
    private File        profiles;
    private FileMonitor monitor;
    private Json.Node?  root = null;
    private bool        timeout;
    private uint        cookie = 0;

    internal Cache() {
      profiles = File.new_for_path(config.cache_path);
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
            warning("Unhandled FileMonitor event");
            return;
        }
        parse_async(true);
      });

      parse();
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
      if (root == null)
        warning ("Root JSON element of profile cache is empty");
      if (root.get_array() == null) {
        warning ("Root JSON element of profile cache is not an array");
        root = null;
      }
    }
  }

  internal class ProfileCacheManager {
    private  File        profiles;
    private  Json.Parser parser;

    internal ProfileCacheManager() {
      profiles = File.new_for_path(config.cache_path);
      parser  = new Parser();
    }

    internal void add_profile_from_data (string profile_data) {
      debug ("Building aggregated profiles.json file for local cache");
      if (parser.load_from_data (profile_data) == false ||
          parser.get_root() == null                     ||
          parser.get_root().get_object() == null) {
        warning("Profile request data was not a valid JSON object: %s", profile_data);
        return;
      }

      var profile = parser.get_root();
      var cache = get_cache_root();
      if (cache == null)
        return;

      cache.get_array().add_element(profile);

      var generator = new Json.Generator();
      generator.pretty = true;
      generator.set_root(cache);
      write(generator.to_data(null));
    }

    public Json.Node? get_cache_root () {
      try {
        parser.load_from_stream (profiles.read(null), null);
      } catch (Error e) {
        warning ("There was an error trying to load the profile cache: %s", e.message);
        return null;
      }

      var root = parser.get_root();

      /* If for some reason the file ends up corrupted we try to restore it */
      if (root == null || root.get_array() == null) {
        warning ("The root element of the cache is not an array: flushing");
        return null;
      }

      return root;
    }

    internal bool flush () {
      debug("Flushing profile cache");
      parser.load_from_data("");
      return write("[]");
    }

    private bool write (string payload) {
      debug("Writing to cache");
      try {
        var w = profiles.replace(null, false, FileCreateFlags.PRIVATE, null);
        var d = new DataOutputStream(w);
        d.put_string(payload);
      } catch (Error e) {
        warning("Could not rewrite %s: %s", profiles.get_path(), e.message);
        return false;
      }

      return true;
    }
  }

  internal class SourceManager {
    private Soup.Session    http_session;
    private Json.Parser     parser;
    internal ProfileCacheManager profiles;

    internal SourceManager(ProfileCacheManager profiles) {
      http_session = new Soup.Session();
      parser = new Json.Parser();
      this.profiles = profiles;

      update_profiles();

      Timeout.add(config.polling_interval * 1000, () => {
        if (config.source == null || config.source == "")
          return true;

        update_profiles ();
        return true;
      });
    }

    private void update_profiles () {
      var msg = new Soup.Message("GET", config.source);
      debug("Queueing request to %s", config.source);
      http_session.queue_message(msg, (s,m) => {
        if (process_json_request(m) == false)
          return;

        var index = (string) m.response_body.data;
        build_profile_cache(index);
        debug ("%s: %s:", m.uri.to_string(true), index);
      });
    }

    private static bool process_json_request(Soup.Message msg) {
      var uri = msg.uri.to_string(true);
      debug("Request to %s finished with status %u", uri, msg.status_code);
      if (msg.response_body == null || msg.response_body.data == null) {
        warning("%s returned no data", uri);
        return false;
      }

      var index = (string) msg.response_body.data;
      if (msg.status_code != 200) {
        warning ("ERROR Message %s: %s", uri, index);
        return false;
      }

      return true;
    }

    private void build_profile_cache(string index) {
      var urls = new string[0];
      try {
        parser.load_from_data(index);
      } catch (Error e) {
        warning ("There was an error trying to parse index: %s", e.message);
        return;
      }

      var root = parser.get_root ();
      switch (root.get_node_type ()) {
        case Json.NodeType.ARRAY:
          break;
        case Json.NodeType.NULL:
          warning("No root element at index");
          return;
        default:
          warning("Root element of index was not of type array");
          return;
      }

      root.get_array().foreach_element((a,i,n) => {

        if (n.get_node_type() != Json.NodeType.OBJECT) {
          warning("Element of the index was not of type object");
          return;
        }

        var index_entry = n.get_object();
        if (index_entry.has_member("url") == false) {
          warning("Index element has no url key");
          return;
        }

        var url_node = index_entry.get_member("url");
        if (url_node.get_node_type() != Json.NodeType.VALUE ||
            url_node.get_value().holds(typeof(string)) == false) {
          warning("Url is not a string");
          return;
        }

        var url = url_node.get_string();
        urls += url;

        debug("URL for profile found: %s", url_node.get_string());
      });

      debug ("Sending requests for URLs in the index");
      profiles.flush();
      foreach (var url in urls) {
        var msg = new Soup.Message("GET", config.source + url);
        http_session.queue_message(msg, profile_response_cb);
      }
    }

    private void profile_response_cb (Soup.Session s, Soup.Message m) {
      if (process_json_request(m) == false)
        return;
      profiles.add_profile_from_data ((string)m.response_body.data);
    }
  }

  internal class ConfigReader
  {
    internal string source           = "";
    internal uint   polling_interval = 60 * 60;
    internal string cache_path       = "/var/cache/fleet-commander/profiles.json";

    internal ConfigReader (string path = "/etc/xdg/fleet-commander.conf") {
      var file = File.new_for_path (path);
      try {
        var dis = new DataInputStream (file.read ());
        string line;

        while ((line = dis.read_line (null)) != null) {
          debug ("LINE '%s'\n", line);
          var field = line.split(":", 2);
          if (field.length != 2)
            continue;

          var key = field[0].strip();
          var val = field[1].strip();

          process_key_value (key, val);
        }
      } catch (Error e) {
        debug ("There was an error reading configuration");
        return;
      }
    }

    internal void process_key_value (string key, string val) {
      switch (key) {
        case "source":
          if (!val.has_suffix ("/"))
            warning ("%s: source must end with /", val);
          if (!val.has_prefix ("http://"))
            warning ("%s: source must start with http://", val);
          source = val;
          break;
        case "polling-interval":
          int tmp = int.parse(val);
          if (tmp == 0)
            break;
          polling_interval = (uint)tmp;
          break;
      }
    }
  }

  public static int main (string[] args) {
    var ml = new GLib.MainLoop (null, false);

    config = new ConfigReader();
    var profmgr = new ProfileCacheManager();
    var cache   = new Cache();
    var srcmgr  = new SourceManager(profmgr);

/*    if (config.source == "") {
      debug("Configuration did not provide profile source");
      return 1;
    }

    try {
      logind = Bus.get_proxy_sync (BusType.SYSTEM,
                                   "org.freedesktop.login1",
                                   "/org/freedesktop/login1");
    } catch (Error e) {
      debug("There was an error trying to connect to Logind in the system bus");
    }*/

    ml.run();
    return 0;
  }
}
