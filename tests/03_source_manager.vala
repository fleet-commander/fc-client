/* Mocked classes */
public const string DEFAULT_URL = "http://foobar/";
public const string PROFILE_INDEX = "[{\"url\": \"123456\"}]";
public const string DEFAULT_PROFILE = "fake_profile_data-123456";

namespace Soup {
  public delegate void SessionCallback (Soup.Session session, Soup.Message msg);

  public struct ResponseBody {
    public string? data;
  }

  public class Uri {
    public string url;
    public string to_string (bool just_path_and_query) {
      return url;
    }
  }

  public class Message {
    public ResponseBody? response_body;
    public Uri           uri;
    public int           status_code;
    public string        method;
    public string        url;

    public Message (string method, string url) {
      this.method = method;
      this.url = url;

      uri = new Uri();
      uri.url = url;
      response_body = ResponseBody ();
    }
  }

  public class Session {
    public Session () {
    }

    public void queue_message (Message msg, SessionCallback callback) {
      msg.status_code = 200;
      if (msg.url == DEFAULT_URL) {
        msg.response_body.data = PROFILE_INDEX;
      } else if (msg.url == DEFAULT_URL + "123456") {
        msg.response_body.data = DEFAULT_PROFILE;
      }
      callback (this, msg);
    }
  }
}

public class ProfileCacheManager {
  private string[] profiles;

  public ProfileCacheManager () {
    profiles = {};
  }

  public bool flush () {
    profiles = {};
    return true;
  }

  public void add_profile_from_data (string data) {
    profiles += data;
  }

  public string[] get_profiles () {
    return profiles;
  }
}

/* Tests */

namespace FleetCommander {
  public delegate void TestFn ();

  public void test_initialization () {
    var pcm = new ProfileCacheManager();
    var sm  = new SourceManager (pcm, DEFAULT_URL, 0);

    assert_nonnull (sm);

    var profiles = pcm.get_profiles ();

    assert (profiles.length == 1);
    assert (profiles[0] == DEFAULT_PROFILE);
  }

  public static void setup () {
  }

  public static void teardown () {
  }

  public static void add_test (string name, TestSuite suite, TestFn fn) {
    suite.add(new TestCase(name, setup, (GLib.TestFunc)fn, teardown));
  }

  public static int main (string[] args) {
    Test.init (ref args);
    var fc_suite = new TestSuite("fleetcommander");
    var pcm_suite = new TestSuite("source-manager");

    add_test ("initialization", pcm_suite, test_initialization);

    fc_suite.add_suite (pcm_suite);
    TestSuite.get_root ().add_suite (fc_suite);
    return Test.run();
  }
}
