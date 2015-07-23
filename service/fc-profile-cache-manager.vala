namespace FleetCommander {
  internal class ProfileCacheManager {
    private  File        profiles;
    private  Json.Parser parser;

    internal ProfileCacheManager(string cache_path) {
      profiles = File.new_for_path(cache_path);
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
        flush ();
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
}
