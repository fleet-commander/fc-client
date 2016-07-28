namespace FleetCommander {
  public class ConfigReader
  {
    public string source             = "";
    public uint   polling_interval   = 60 * 60;
    public string cache_path         = "/var/cache/fleet-commander";
    public string dconf_db_path      = "/etc/dconf/db";
    public string dconf_profile_path = "/run/dconf/user";
    public string goa_run_path       = "/run/goa-1.0";

    public ConfigReader (string path = "/etc/xdg/fleet-commander.conf") {
      var keyfile = new KeyFile ();
      try {
        keyfile.load_from_file (path, KeyFileFlags.NONE);
      } catch (Error e) {
        warning ("Error parsing configuration file %s: %s", path, e.message);
        return;
      }

      if (keyfile.has_group("fleet-commander") == false) {
        warning ("Configuration file %s does not have [fleet-commander] group", path);
        return;
      }

      string[] keys = {"source", "polling-interval", "cache-path", "dconf-db-path", "dconf-profile-path"};
      foreach (string key in keys) {
        if (keyfile.has_key ("fleet-commander", key) == false)
          continue;

        try {
          var val = keyfile.get_string ("fleet-commander", key);
          process_key_value (key, val);
        } catch (Error e) {
          warning ("There was an error reading key %s from %s: %s", key, path, e.message);
          continue;
        }
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
          if (tmp == 0) {
            warning ("Value 0 is invalid for polling-interval");
            break;
          }
          polling_interval = (uint)tmp;
          break;
        case "cache-path":
          cache_path = val;
          break;
        case "dconf-db-path":
          dconf_db_path = val;
          break;
        case "dconf-profile-path":
          dconf_profile_path = val;
          break;
        case "goa-run-path":
          goa_run_path = val;
          break;
      }
    }
  }
}
