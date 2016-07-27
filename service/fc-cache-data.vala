namespace FleetCommander {
  public class CacheData {
    private Json.Node?     root = null;
    private File           profiles;
    private ContentMonitor monitor;

    public signal void parsed();

    public CacheData (string path) {
      profiles = File.new_for_path (path);
      monitor = new ContentMonitor (profiles.get_path ());
      monitor.content_updated.connect (() => {
        parse ();
      });
      parse ();
    }

    public string get_path () {
      return profiles.get_path();
    }

    public Json.Node? get_root() {
      return root;
    }

    public Json.Object? get_profile (string uid) {
      var profiles = get_profiles ({uid});

      if (profiles == null || profiles.length < 1)
        return null;

      return profiles[0];
    }

    //FIXME: This probably belongs in another class
    public Json.Object[]? get_profiles (string[] uids) {
      GenericArray<Json.Object?> result_array = new GenericArray<Json.Object?> ();

      if (root == null) {
        warning ("Could not read JSON data from profile cache");
        return null;
      }

      if (root.get_node_type () != Json.NodeType.ARRAY) {
        warning ("Root JSON object was not an array");
        return null;
      }

      var profiles = root.get_array ();
      
      Json.Object? result = null;

      for (uint i = 0; i < profiles.get_length (); i++) {
        var n = profiles.get_element (i);
        if (n == null)
          continue;
        if (n.get_node_type () != Json.NodeType.OBJECT)
          continue;

        var profile = n.get_object ();
        if (!profile.has_member ("uid"))
          continue;

        var profile_uid = profile.get_string_member ("uid");

        if (profile_uid == null)
          continue;

        foreach (string requested_uid in uids) {
          if (requested_uid == profile_uid)
            result_array.add (profile);
        }
      }

      return result_array.data;
    }

    private void parse () {
      if (profiles.query_exists () == false) {
        debug ("cache file %s: does not exist", profiles.get_path ());
        root = null;
        parsed ();
        return;
      }

      debug ("Parsing cache file: %s", profiles.get_path());
      var parser = new Json.Parser();
      try {
        parser.load_from_stream (profiles.read());
      } catch (Error e) {
        warning ("There was an error parsing %s: %s", profiles.get_path(), e.message);
        return;
      }

      root = parser.get_root();
      if (root == null) {
        warning ("Root JSON element of profile cache is empty");
      }

      parsed();
    }
  }
}
