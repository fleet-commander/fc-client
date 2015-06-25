using Soup;
using Json;

namespace Logind {
  internal struct User {
    uint32 uid;
    string name;
    ObjectPath path;
  }

  [DBus (name = "org.freedesktop.login1.Manager")]
  interface Manager : GLib.Object {
    public abstract User[] list_users () throws IOError;
    public abstract signal void user_new     (uint32 uid, ObjectPath path);
    public abstract signal void user_removed (uint32 uid, ObjectPath path);
  }
}

namespace FleetCommander {
  internal ConfigReader config;

  private class UserSessionHandler {
    private UserIndex index;
    private Logind.Manager? logind;
    private List<string> merged_profiles = null;

    internal UserSessionHandler (UserIndex index) {
      this.index = index;
      logind = null;

      Bus.watch_name (BusType.SYSTEM,
                      "org.freedesktop.login1",
                      BusNameWatcherFlags.AUTO_START,
                      bus_name_appeared_cb,
                      (con, name) => {
                        warning ("org.freedesktop.login1 bus name vanished");
                        logind = null;
                      });
      index.get_cache().parsed.connect (update_dconf_profiles);
    }

     private void update_dconf_profiles () {
      if (logind == null)
        return;

      if (dconf_db_can_write () == false)
        return;
      try {
        foreach (var user in logind.list_users()) {
          create_dconf_profile_for_user (user.uid);
        }
      } catch (IOError e) {
        warning ("There was an error calling ListUsers in /org/freedesktop/login1");
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
      update_dconf_profiles ();
    }

    private void user_logged_cb (uint32 user_id, ObjectPath path) {
      if (dconf_db_can_write () == false)
        return;

      debug ("User logged in with uid: %u", user_id);
      create_dconf_profile_for_user (user_id);
      index.flush ();
    }

    private void create_dconf_profile_for_user (uint32 uid) {
      var dconf_profile_data = "user-db:user";

      var groups = Posix.get_groups_for_user (uid);
      var user_index  = index.get_user_profiles ();
      var group_index = index.get_group_profiles ();
      var name = Posix.uid_to_user_name (uid);

      if (name == null) {
        warning ("%u user has no user name string", uid);
        return;
      }

      merged_profiles = null;

      if (user_index != null         &&
          user_index.has_member (name)) {
        var profiles = user_index.get_array_member (name);
        if (profiles != null) {
          profiles.foreach_element (add_elements_to_merged_profiles);
        }
      }

      /* For each group that the user belongs to we check
       * if there is are profiles assigned to it. */
      if (group_index != null      &&
          group_index.get_size () > 0) {
        foreach (var group in groups) {
          if (group_index.has_member(group) == false)
            continue;

          var profiles = group_index.get_array_member (group);
          if (profiles == null)
            continue;
          profiles.foreach_element (add_elements_to_merged_profiles);
        }
      }

      if (merged_profiles.length() < 1) {
        delete_dconf_profile (name);
        merged_profiles = null;
        return;
      }

      foreach (var profile in merged_profiles) {
        dconf_profile_data += "\nsystem-db:%s".printf("fleet-commander-" + profile);
      }
      merged_profiles = null;

      write_dconf_profile (name, dconf_profile_data);
    }

    private void delete_dconf_profile (string name) {
      var path = string.join("/", config.dconf_profile_path, "u:" + name);
      try {
        var obsolete_profile = File.new_for_path (path);
        if (obsolete_profile.query_exists() == true)
          obsolete_profile.delete ();
      } catch (Error e) {
        warning ("could not remove unused profile %s", path);
      }
    }

    private void write_dconf_profile (string user_name, string data) {
      var path = string.join("/", config.dconf_profile_path, "u:" + user_name);
      debug ("Attempting to write dconf profile in %s", path);
      try {
        var dconf_profile = File.new_for_path (path);
        var w = dconf_profile.replace(null, true, FileCreateFlags.NONE, null);
        var d = new DataOutputStream(w);
        d.put_string(data);
      } catch (Error e) {
        warning("Could not rewrite %s: %s", path, e.message);
        return;
      }
    }

    private void add_elements_to_merged_profiles (Json.Array a, uint i, Json.Node n) {
      var profile = n.get_string();
      if (profile == null) {
        warning ("user -> profiles index contained an element that is not a string");
        return;
      }

      if (merged_profiles.find_custom (profile, GLib.strcmp) == null) {
        merged_profiles.prepend (profile);
      }
    }
  }

  private static bool object_has_members (Json.Object object, string[] keys) {
    bool all = true;
    var members = object.get_members();
    foreach (var key in keys) {
      all = members.find_custom (key, (a,b) => { return (a == b)? 0 : 1; }) != null;
      if (!all) {
        debug ("key %s not found", key);
        break;
      }
    }
    return all;
  }

  internal class UserIndex {
    private Json.Object user_profiles;
    private Json.Object group_profiles;
    private CacheData cache;
    private bool      index_built;

    internal UserIndex (CacheData cache) {
      this.cache = cache;
      index_built = false;
    }

    internal CacheData get_cache () {
      return cache;
    }

    private void rebuild_index () {
      debug ("%s: grabbing user/groups", config.cache_path);
      flush ();

      if (cache.get_root() == null) {
        warning ("%s is empty or there was some error parsing it", config.cache_path);
        return;
      }
      var profiles = cache.get_root().get_array();
      if (profiles == null) {
        warning("%s does not contain a JSON list as root element", config.cache_path);
        return;
      }

      profiles.foreach_element ((a,i,n) => {
        var profile = n.get_object();
        if (profile == null) {
          warning ("%s contains an element that is not a profile", config.cache_path);
          return;
        }

        var uid = profile.get_string_member("uid");
        if (uid == null) {
          warning ("%s #%u profile's uid member is not a string", uid, i);
          return;
        }

        var applies_to = profile.get_object_member("applies-to");
        if (applies_to == null) {
          warning ("%s profile: 'applies-to' holds no object", uid);
          return;
        }

        string[] keys = {"users", "groups"};
        foreach (string key in keys) {
          var list = applies_to.get_array_member (key);
          if (list == null) {
            warning ("%s profile: '%s' member is not an array", uid, key);
            continue;
          }

          var collection = key == "users"? user_profiles : group_profiles;

          list.foreach_element ((a, i, n) => {
            var user_or_group = n.get_string();
            if (user_or_group == null) {
              warning ("%s[applies-to][%s] #%u array element was not a string", uid, key, i);
              return;
            }

            Json.Array? uids = null;
            if (collection.has_member(user_or_group) == false ||
                (uids = collection.get_array_member(user_or_group)) == null) {
              uids = new Json.Array ();
              collection.set_array_member (user_or_group, uids);
            }

            uids.add_string_element (uid);
          });
        }
      });

      index_built = true;
    }

    internal Json.Object? get_user_profiles () {
      if (index_built == false)
        rebuild_index ();
      return user_profiles;
    }

    internal Json.Object? get_group_profiles () {
      if (index_built == false)
        rebuild_index ();
      return user_profiles;
    }

    internal void flush () {
      debug ("Flusing UserIndex database");
      user_profiles = new Json.Object ();
      group_profiles = new Json.Object ();

      index_built = false;
    }
  }

  internal static bool dconf_db_can_write () {
    if (FileUtils.test (config.dconf_db_path, FileTest.EXISTS) == false) {
      warning ("dconf datbase path %s does not exists", config.dconf_db_path);
      return false;
    }
    if (Posix.access(config.dconf_db_path, Posix.W_OK | Posix.X_OK) != 0) {
      warning ("Cannot write data onto %s", config.dconf_db_path);
      return false;
    }
    return true;
  }

  internal class DconfDbWriter {
    private CacheData cache;
    private string[] VALIDATED_KEYS = {"uid", "settings"};

    internal DconfDbWriter(CacheData cache) {
      this.cache = cache;

      this.cache.parsed.connect(update_databases);
    }

    internal void update_databases() {
      debug("Updating dconf databases");
      var root = cache.get_root();
      if (root == null)
        return;

      if (dconf_db_can_write() == false)
        return;

      remove_current_profiles ();

      root.get_array().foreach_element ((a, i, n) => {
        //FIXME: Use etag to check if we need to replace this profile
        var profile = n.get_object();
        if (profile == null) {
          warning("Cached profile is not a JSON object (element #%u)", i);
          return;
        }

        if (object_has_members (profile, VALIDATED_KEYS) == false) {
          warning ("Could not find mandatory keys in cached profile #%u", i);
          return;
        }

        commit_profile(profile);
      });

      call_dconf_update();
    }

    private void remove_current_profiles () {
      var db_path               = Dir.open (config.dconf_db_path);
      string[] current_profiles = {};

      var root = cache.get_root ();
      if (root != null && root.get_array () != null) {
        root.get_array ().foreach_element ((a, i, n) => {
          var profile = n.get_object ();
          if (profile == null)
            return;
          if (profile.has_member ("uid") == false)
            return;
          current_profiles += profile.get_string_member ("uid");
        });
      }

      for (var db_child = db_path.read_name (); db_child != null; db_child = db_path.read_name ()) {
        bool exists = false;

        if (db_child.has_prefix ("fleet-commander-") == false)
           continue;

        var path = string.join ("/", config.dconf_db_path, db_child);

        if (db_child.has_suffix (".d") && FileUtils.test (path, FileTest.IS_DIR)) {
          foreach (var existing in current_profiles) {
            if (path.has_suffix (existing + ".d") == false)
              continue;
            exists = true;
            break;
          }
          if (exists)
            continue;

          var keyfile  = string.join("/", path,  "generated");
          var locks    = string.join("/", path,  "locks");
          var lockfile = string.join("/", locks, "generated");
          FileUtils.remove (keyfile);
          FileUtils.remove (lockfile);
          FileUtils.remove (locks);
          FileUtils.remove (path);
          continue;
        }

        if (FileUtils.test (path, FileTest.IS_REGULAR) == false)
          continue;

        /* Find out if this profile is still valid */
        foreach (var existing in current_profiles) {
          if (path.has_suffix (existing) == false)
            continue;
          exists = true;
          break;
        }
        if (exists)
          continue;

        debug ("Removing profile database %s", path);
        if (FileUtils.remove(path) == -1) {
          warning ("There was an error attempting to remove %s", path);
        }
      }
    }

    private void call_dconf_update () {
      var dconf_update = new Subprocess.newv ({"dconf", "update"}, SubprocessFlags.NONE);
      dconf_update.wait ();

      if (dconf_update.get_if_exited () == false) {
        warning ("dconf update process did not exit");
        dconf_update.force_exit ();
        return;
      }

      if (dconf_update.get_exit_status () != 0) {
        warning ("dconf update failed");
      }
    }

    /* This function makes sure that we can write the keyfile for a profile */
    internal bool check_filesystem_for_profile (File settings_keyfile) {
      debug("Checking whether we can write profile data to %s", settings_keyfile.get_path ());

      if (settings_keyfile.query_exists() == true) {
        var info = settings_keyfile.query_info("access::can-delete", FileQueryInfoFlags.NONE);
        if (info.get_attribute_boolean("access::can-delete") == false) {
          warning ("Could not replace file %s", settings_keyfile.get_path ());
          return false;
        }
      }

      var basedir = settings_keyfile.get_parent().get_path();
      if (FileUtils.test (basedir, FileTest.EXISTS) == false) {
        if (DirUtils.create_with_parents (basedir, 0755) == -1) {
          warning ("Could not create directory %s", basedir);
          return false;
        }
      }
      if (FileUtils.test (basedir, FileTest.IS_DIR) == false) {
        warning ("%s exists but is not a file", basedir);
        return false;
      }

      if (Posix.access(basedir, Posix.W_OK | Posix.X_OK) != 0) {
        warning ("Cannot write data onto %s", basedir);
        return false;
      }

      return true;
    }

    internal void commit_profile (Json.Object profile) {
      debug("Committing profile");
      string?           uid;
      Json.Object?      settings;
      Json.Array?       gsettings;
      File              generated;

      if ((uid = profile.get_member("uid").get_string()) == null) {
        warning ("profile uid JSON member was not a string");
        return;
      }

      /* We make sure we can write this profile and go ahead*/
      generated = File.new_for_path (string.join("/", config.dconf_db_path, "fleet-commander-" + uid + ".d",
                                                 "generated"));
      if (check_filesystem_for_profile (generated) == false)
        return;

      /* We make sure the JSON object is sane */
      if ((settings = profile.get_member("settings").get_object()) == null) {
        warning ("profile %s: 'settings' JSON member is not a JSON object", uid);
        return;
      }
      var node = settings.get_member("org.gnome.gsettings");
      if (node == null) {
        debug("profile %s: does not have any GSettings", uid);
        return;
      } else if ((gsettings = node.get_array()) == null) {
        warning ("profile %s: org.gnome.gsettings member of settings is not a JSON array", uid);
        return;
      }

      var keyfile = new KeyFile();
      var schema_source = SettingsSchemaSource.get_default();
      gsettings.foreach_element((a, i, n) => {
        var change = n.get_object();
        if (change == null) {
          warning ("profile %s: gsettings change item #%u should be a JSON object", uid, i);
          return;
        }

        if (object_has_members (change, {"key", "value"}) == false) {
          warning ("profile %s: Could not find mandatory keys in change %u", uid, i);
          return;
        }

        var key = change.get_member("key").get_string();
        if (key == null) {
          warning ("profile %s: 'key' member of change %u is not a string", uid, i);
        }

        string? signature = null;
        var schema_key  = GLib.Path.get_basename (key);
        var schema_path = GLib.Path.get_dirname  (key);
        schema_path = schema_path.slice(1,schema_path.length);
        /* TODO: get the logger to send the signature */
        if (change.has_member ("signature")) {
            signature = change.get_string_member ("signature");
        }
        if (signature == null && change.has_member("schema")) {
          //TODO: Remove this after a few releases
          warning ("profile %s: 'signature' string was not present in change %u", uid, i);
          var schema_id = change.get_member("schema").get_string();
          if (schema_id == null) {
            warning("profile %s: 'schema' key is not a string in change %u", uid, i);
          } else {
            var schema = schema_source.lookup (schema_id, true);
            if (schema != null && schema.has_key (schema_key)) {
              signature = schema.get_key (schema_key).get_value_type ().dup_string ();
            }
          }
        }

        if (signature == null) {
          warning ("profile %s: could not find signature for key %s", uid, key);
        }

        var variant = Json.gvariant_deserialize(change.get_member("value"), signature);
        if (variant == null) {
          warning ("profile %s: could not deserialize JSON to GVariant for key %u", key, i);
          return;
        }

        keyfile.set_string(schema_path, schema_key, variant.print(true));
      });

      debug("Saving profile keyfile in %s", generated.get_path());
      try {
        keyfile.save_to_file (generated.get_path ());
      } catch (Error e) {
        warning ("There was an error saving profile settings in %s", generated.get_path ());
      }
    }
  }

  internal class CacheData {
    private File        profiles;
    private FileMonitor monitor;
    private Json.Node?  root = null;
    private bool        timeout;
    private uint        cookie = 0;

    internal signal void parsed();

    internal CacheData () {
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
            debug("Unhandled FileMonitor event");
            return;
        }
        parse_async(true);
      });

      parse();
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
    internal string source             = "";
    internal uint   polling_interval   = 60 * 60;
    internal string cache_path         = "/var/cache/fleet-commander/profiles.json";
    internal string dconf_db_path      = "/etc/dconf/db";
    internal string dconf_profile_path = "/etc/dconf/profile";

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
    var srcmgr  = new SourceManager(profmgr);

    var cache   = new CacheData();
    var dconfdb = new DconfDbWriter(cache);
    var uindex  = new UserIndex(cache);
    var usermgr = new UserSessionHandler(uindex);

    ml.run();
    return 0;
  }
}
