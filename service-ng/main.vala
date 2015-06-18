using Soup;

namespace Logind {

  internal struct User {
    uint32 uid;
    string name;
    string path;
  }

  [DBus (name = "org.freedesktop.login1.Manager")]
  interface Manager : Object {
    public abstract User[] list_users () throws IOError;
    public abstract signal void user_new     (uint32 uid, ObjectPath path);
    public abstract signal void user_removed (uint32 uid, ObjectPath path);
  }
}

namespace FleetCommander {
  internal ConfigReader config;
  internal Logind.Manager? logind;

  internal class SourceManager {
    private Soup.Session http_session;

    internal SourceManager() {
      http_session = new Soup.Session();
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
        debug("Request finished with status %u", m.status_code);
      });
    }
  }

  internal class ConfigReader
  {
    internal string source = "";
    internal uint    polling_interval = 60 * 60;
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
    var srcmgr = new SourceManager();

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
