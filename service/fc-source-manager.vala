namespace FleetCommander {
  internal class SourceManager {
    private Soup.Session         http_session;
    private Json.Parser          parser;
    internal ProfileCacheManager profiles;
    private  string              source;

    internal SourceManager(ProfileCacheManager profiles,
                           string              source,
                           uint                polling_interval) {
      http_session = new Soup.Session();
      parser = new Json.Parser();
      this.profiles = profiles;
      this.source = source;

      if (this.source.has_suffix ("/") == false)
        this.source = this.source + "/";

      update_profiles();

      Timeout.add(polling_interval * 1000, () => {
        if (source == null || source == "")
          return true;

        update_profiles ();
        return true;
      });
    }

    private void update_profiles () {
      profiles.flush ();

      var profiles_msg = new Soup.Message("GET", source + "index.json");
      debug("Queueing request to %s", profiles_msg.uri.to_string (false));
      http_session.queue_message(profiles_msg, (s,m) => {
        if (process_json_request(m) == false)
          return;

        var index = (string) m.response_body.data;
        build_profile_cache(index);
        debug ("%s: %s:", m.uri.to_string(true), index);
      });

      get_applies ();
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
      foreach (var url in urls) {
        var msg = new Soup.Message("GET", source + url);
        http_session.queue_message(msg, profile_response_cb);
      }
    }

    private void get_applies () {
      var applies_msg = new Soup.Message ("GET", source + "applies.json");
      debug("Queueing request to %s", applies_msg.uri.to_string (false));
      http_session.queue_message (applies_msg, (s,m) => {
        if (process_json_request(m) == false)
          return;

        var applies = (string) m.response_body.data;
        profiles.write_applies (applies);
        debug ("%s: %s:", m.uri.to_string(true), applies);
      });
    }

    private void profile_response_cb (Soup.Session s, Soup.Message m) {
      if (process_json_request(m) == false)
        return;
      profiles.add_profile_from_data ((string)m.response_body.data);
    }
  }
}
