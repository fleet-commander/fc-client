namespace FleetCommander {
  public class UserIndex {
    private Json.Object? user_profiles = null;
    private Json.Object? group_profiles = null;
    private CacheData cache;
    private bool      index_built;

    public signal void updated ();

    public UserIndex (CacheData cache) {
      this.cache = cache;
      index_built = false;

      this.cache.parsed.connect(() => {
        flush ();
        updated ();
      });
    }

    private void rebuild_index () {
      debug ("%s: grabbing user/groups", cache.get_path ());
      flush ();

      if (cache.get_root() == null) {
        debug ("%s is empty or there was some error parsing it", cache.get_path ());
        return;
      }
      var applies = cache.get_root().get_object();
      if (applies == null) {
        warning("%s does not contain a JSON object as root element", cache.get_path ());
        return;
      }

      applies.foreach_member ((o, uid, n) => {
        var profile_applies = n.get_object();
        if (profile_applies == null) {
          warning ("%s contains an element that is not a profile", cache.get_path ());
          return;
        }

        string[] keys = {"users", "groups"};
        foreach (string key in keys) {
          var list = profile_applies.get_array_member (key);
          if (list == null) {
            warning ("%s profile applies: '%s' member is not an array", uid, key);
            continue;
          }

          var collection = key == "users"? user_profiles : group_profiles;

          list.foreach_element ((a, i, n) => {
            var user_or_group = n.get_string();
            if (user_or_group == null) {
              warning ("%s[%s] #%u array element was not a string", uid, key, i);
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

    internal string[] get_profiles_for_user_and_groups (string user, List<string> groups) {
      if (index_built == false)
        rebuild_index ();

      var profiles = new GenericArray<string> ();

      if (user_profiles != null && user_profiles.has_member (user)) {
        var uids = user_profiles.get_array_member (user);
        uids.foreach_element ((a, i, n) => {
          if (n == null || n.get_node_type() != Json.NodeType.VALUE)
            return;

          append_dedup (profiles, n.get_string ());
        });
      }
      if (group_profiles != null) {
        foreach (var group in groups) {
          if (!group_profiles.has_member (group))
            continue;

          var uids = group_profiles.get_array_member (group);
          uids.foreach_element ((a, i, n) => {
            if (n == null || n.get_node_type () != Json.NodeType.VALUE)
              return;

            append_dedup (profiles, n.get_string ());
          });
        }
      }

      return profiles.data;
    }

    private static void append_dedup (GenericArray<string> list, string new_element) {
      foreach (var element in list.data) {
        if (element == new_element)
          return;
      }
      list.add (new_element);
    }

    internal Json.Object? get_user_profiles () {
      if (index_built == false)
        rebuild_index ();
      return user_profiles;
    }

    internal Json.Object? get_group_profiles () {
      if (index_built == false)
        rebuild_index ();
      return group_profiles;
    }

    internal void flush () {
      debug ("Flusing UserIndex database");
      user_profiles = new Json.Object ();
      group_profiles = new Json.Object ();

      index_built = false;
    }
  }
}
