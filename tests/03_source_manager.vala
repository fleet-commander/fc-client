/* Mocked classes */
namespace Soup {
  public delegate void SessionCallback (Soup.Session session, Soup.Message msg);

  public struct ResponseBody {
    public string data {get; set;}
  }

  public class Uri {
    public string to_string (bool just_path_and_query) {
      return "";
    }
  }

  public class Message {
    public ResponseBody? response_body;
    public Uri          uri;
    public int          status_code;
    public Message (string method, string url) {
    }
  }

  public class Session {
    public Session () {
    }

    public void queue_message (Message msg, SessionCallback callback) {
    }
  }
}

public class ProfileCacheManager {
  public ProfileCacheManager () {
  }

  public bool flush () {
    return false;
  }

  public void add_profile_from_data (string data) {
  }
}

/* Tests */

namespace FleetCommander {
  public delegate void TestFn ();

  public void test_construct () {
    var sm = new SourceManager (new ProfileCacheManager(), "http://foobar", 10);
    assert_nonnull (sm);
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

    add_test ("construct", pcm_suite, test_construct);

    fc_suite.add_suite (pcm_suite);
    TestSuite.get_root ().add_suite (fc_suite);
    return Test.run();
  }
}
