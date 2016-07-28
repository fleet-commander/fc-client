namespace FleetCommander {
  //Mock CacheData
  public class CacheData {
    private Json.Array?  profiles;
    private Json.Object? applies;

    private string      cache_path;

    public signal void parsed();

    public CacheData () {
      this.cache_path = Environment.get_variable ("FC_PROFILES_PATH");
      profiles = null;
      applies = null;
    }

    public string get_path () {
      return cache_path;
    }

    public void add_profile (string profile) throws Error {
      if (profiles == null) {
        applies = null;
        profiles = new Json.Array ();
      }

      var parser = new Json.Parser ();
      parser.load_from_file (string.join("/", this.cache_path, profile + ".json"));

      var root = parser.get_root ();
      if (root == null)
        return;

      profiles.add_element (parser.get_root ());
    }

    public void set_applies (string applies_file) {
      if (applies == null) {
        profiles = null;
      }

      var parser = new Json.Parser ();
      parser.load_from_file (string.join("/", this.cache_path, applies_file + ".json"));

      var root = parser.get_root ();
      if (root == null)
        return;

      applies = root.get_object ();
    }

    public Json.Object? get_profile (string uid) {
      var profiles = get_profiles ({uid});
      if (profiles == null)
        return null;
      return profiles[0];
    }

    public Json.Object[]? get_profiles (string[] uids) {
      var result = new GenericArray<Json.Object> ();

      if (profiles == null)
        return null;

      profiles.foreach_element ((a, i, n) => {
        foreach (var uid in uids) {
          if (uid != n.get_object().get_string_member ("uid"))
            return;
          result.add (n.get_object ());
        }
      });

      return result.data;
    }

    public Json.Node? get_root () {
      if (profiles != null) {
        var root = new Json.Node (Json.NodeType.ARRAY);
        root.set_array (profiles);
        return root;
      }

      if (applies != null) {
        var root = new Json.Node (Json.NodeType.OBJECT);
        root.set_object (applies);
        return root;
      }

      return null;
    }
  }
}
