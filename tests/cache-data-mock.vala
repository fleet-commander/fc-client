namespace FleetCommander {
  //Mock CacheData
  public class CacheData {
    private Json.Array array;
    private string     cache_path;

    public signal void parsed();

    public CacheData () {
      this.cache_path = Environment.get_variable ("FC_PROFILES_PATH");
      array = new Json.Array ();
    }

    public string get_path () {
      return cache_path;
    }

    public void add_profile (string profile) throws Error {
      //TODO: Open "profile".json file, parse and add to array
      var parser = new Json.Parser ();
      parser.load_from_file (string.join("/", this.cache_path, profile + ".json"));

      var root = parser.get_root ();
      if (root == null)
        return;

      array.add_element (parser.get_root ());
    }

    public Json.Node? get_root () {
      var root = new Json.Node (Json.NodeType.ARRAY);
      root.set_array (array);
      return root;
    }
  }
}
