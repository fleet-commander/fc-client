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

    private Json.Node? get_cache_root () {
      try {
        parser.load_from_stream (profiles.read(null), null);
      } catch (Error e) {
        warning ("Could not load JSON data from %s: %s", profiles.get_path (), e.message);
        return null;
      }
      return parser.get_root ();
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

    public bool write_profiles (Json.Object[] profiles_objs) {
      var gen = new Json.Generator ();
      var node = new Json.Node (Json.NodeType.ARRAY);
      var arr = new Json.Array ();
      foreach (var p in profiles_objs) {
        arr.add_object_element (p);
      }

      node.set_array (arr);
      gen.set_root (node);

      try {
        debug("Writing %s", profiles.get_path ());
        gen.to_file (profiles.get_path ());
      } catch (Error e) {
        warning ("Could not write profiles.json cache file: %s", e.message);
        return false;
      }
      return true;
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
