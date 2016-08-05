namespace FleetCommander {
  internal class SourceManager {
    private Soup.Session         http_session;
    internal ProfileCacheManager profiles;
    private  string              source;

    private bool active_session = false;
    private bool got_applies = false;
    private uint profile_counter;
    private GenericArray<Json.Object> profiles_list;
    private string applies_data;

    internal SourceManager(ProfileCacheManager profiles,
                           string              source,
                           uint                polling_interval) {

      profiles_list = new GenericArray<Json.Object> ();

      http_session = new Soup.Session();
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
      if (active_session) {
        debug ("We are currently in the middle of a session to grab data from the server");
        return;
      }

      /* Bootsrap session control variables */
      active_session = true;
      got_applies = false;
      profile_counter = 0;
      profiles_list.length = 0;

      var profiles_msg = new Soup.Message("GET", source + "index.json");
      debug("Queueing request to %s", profiles_msg.uri.to_string (false));
      http_session.queue_message(profiles_msg, (s,m) => {
        debug ("index.json response with status %u", m.status_code);
        if (process_json_request(m) == false) {
          active_session = false;
          return;
        }

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
      var parser = new Json.Parser ();
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
      profile_counter = 0;
      foreach (var url in urls) {
        profile_counter++;
        var msg = new Soup.Message("GET", source + url);
        http_session.queue_message(msg, profile_response_cb);
      }
      
      get_applies ();
    }

    private void get_applies () {
      got_applies = false;
      applies_data = "";

      var applies_msg = new Soup.Message ("GET", source + "applies.json");
      debug("Queueing request to %s", applies_msg.uri.to_string (false));
      http_session.queue_message (applies_msg, (s,m) => {
        if (process_json_request(m) == false) {
          cancel_session ();
          return;
        }

        applies_data = (string) m.response_body.data;
        debug ("%s: %s:", m.uri.to_string(true), applies_data);

        got_applies = true;

        if (profile_counter == 0)
          commit_session ();
      });
    }

    private void profile_response_cb (Soup.Session s, Soup.Message m) {
      Json.Object? profile = null;
      if (active_session == false) {
        debug ("Response fron %s ignored, the fetch session was aborted", m.uri.get_path ());
        return;
      }

      if (process_json_request(m) == false) {
        cancel_session ();
        return;
      }

      var parser = new Json.Parser ();
      try {
        parser.load_from_data ((string)m.response_body.data, -1);
      } catch (Error e) {
        warning ("Could not parse %s: %s", m.uri.get_path (), e.message);
        cancel_session ();
        return;
      }

      var root = parser.get_root ();
      profile = root.get_object ();
      if (profile == null) {
        warning ("Root JSON object of profile %s was not an object", m.uri.get_path ());
        cancel_session ();
      }
      profiles_list.add (profile);

      profile_counter--;
      if (profile_counter == 0) {
        debug ("All profiles fetched");
        if (got_applies)
          commit_session ();
      }
    }

    private void commit_session () {
      debug ("Committing data to profiles.json and applies.json");
      active_session = false;
      profiles.write_applies (applies_data);
      profiles.write_profiles (profiles_list.data);
    }

    private void cancel_session () {
      debug ("Cancelling fetching session");
      active_session = false;
      profiles_list.length = 0;
      applies_data = "";
    }
  }
}
