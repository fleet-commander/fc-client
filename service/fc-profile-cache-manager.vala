namespace FleetCommander {
  public class ProfileCacheManager {
    private  File        profiles;
    private  File        applies;
    private  Json.Parser parser;

    public ProfileCacheManager(string cache_path) {
      profiles = File.new_for_path (cache_path + "/profiles.json");
      applies  = File.new_for_path (cache_path + "/applies.json");
      parser  = new Json.Parser ();
    }

    public void add_profile_from_data (string profile_data) {
      debug ("Building aggregated profiles.json file for local cache");

      var profiles_cache = get_profiles_node();
      if (profiles_cache == null)
        return;

      try {
        parser.load_from_data (profile_data);
      } catch (Error e) {
        warning ("Could not parse %s: %s", profile_data, e.message);
        return;
      }

      if (parser.get_root() == null                     ||
          parser.get_root().get_object() == null) {
        warning("Profile request data was not a valid JSON object: %s", profile_data);
        return;
      }

      var profile = parser.get_root();

      profiles_cache.get_array ().add_element(profile);

      var generator = new Json.Generator();
      generator.pretty = true;
      generator.set_root(profiles_cache);
      write_profiles (generator.to_data(null));
    }

    private Json.Node? get_cache_root () {
      try {
        parser.load_from_stream (profiles.read(null), null);
      } catch (Error e) {
        warning ("Could not load JSON data from %s: %s", profiles.get_path (), e.message);
        return null;
      }
      return parser.get_root ();
    }

    private Json.Node? get_profiles_node () {
      if (profiles.query_exists () == false) {
        debug ("%s does not exists, creating an empty one", profiles.get_path ());
        if (write_profiles ("[]") == false)
          return null;
      }

      var root = get_cache_root ();
      if (root == null || root.get_array() == null) {
        warning ("The root element of the cache is not an array: flushing");
        flush ();
        return null;
      }

      return root;
    }

    public void flush () {
      debug("Flushing profile cache");

      if (profiles.query_exists (null)) {
        try {
          profiles.delete ();
        } catch (Error e) {
          warning ("Could not delete %s: %s", profiles.get_path (), e.message);
        }
      }

      if (applies.query_exists ()) {
        try {
          applies.delete ();
        } catch (Error e) {
          warning ("Could not delete %s: %s", applies.get_path (), e.message);
        }
      }
    }

    private bool write_profiles (string payload) {
      return write_to_file (profiles, payload);
    }

    public void write_applies (string payload) {
      write_to_file (applies, payload);
    }

    private bool write_to_file (File file, string payload) {
      debug("Writing %s", file.get_path ());
      try {
        var w = file.replace(null, false, FileCreateFlags.PRIVATE, null);
        var d = new DataOutputStream(w);
        d.put_string(payload);
      } catch (Error e) {
        warning("Could not rewrite %s: %s", file.get_path(), e.message);
        return false;
      }

      return true;
    }
  }
}
