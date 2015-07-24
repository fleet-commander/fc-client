namespace FleetCommander {
  internal class ProfileCacheManager {
    private  File        profiles;
    private  Json.Parser parser;

    internal ProfileCacheManager(string cache_path) {
      profiles = File.new_for_path(cache_path + "/profiles.json");
      parser  = new Json.Parser();
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

    private Json.Node? get_cache_root_private () {
      try {
        parser.load_from_stream (profiles.read(null), null);
      } catch (Error e) {
        warning ("Could not load JSON data from %s: %s", profiles.get_path (), e.message);
        return null;
      }
      return parser.get_root ();
    }

    public Json.Node? get_cache_root () {
      if (profiles.query_exists () == false) {
        if (flush () == false)
          return null;
      }

      var root = get_cache_root_private ();
      if (root == null || root.get_array() == null) {
        message ("The root element of the cache is not an array: flushing");
        if (flush () == false)
          return null;
        return get_cache_root_private ();
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
}
