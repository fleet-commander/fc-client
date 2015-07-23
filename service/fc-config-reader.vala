namespace FleetCommander {
  public class ConfigReader
  {
    public string source             = "";
    public uint   polling_interval   = 60 * 60;
    public string cache_path         = "/var/cache/fleet-commander/profiles.json";
    public string dconf_db_path      = "/etc/dconf/db";
    public string dconf_profile_path = "/etc/dconf/profile";

    public ConfigReader (string path = "/etc/xdg/fleet-commander.conf") {
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

    private void process_key_value (string key, string val) {
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
}
