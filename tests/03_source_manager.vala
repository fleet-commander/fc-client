/* Mocked classes */
public const string DEFAULT_URL = "http://foobar/";
public const string PROFILE_INDEX = "[{\"url\": \"123456.json\"}]";
public const string DEFAULT_PROFILE = "fake_profile_data-123456";
public const string DEFAULT_APPLIES = "{}";

public MainLoop? loop = null;
public uint index_requests;
public uint profile_requests;
public uint applies_requests;

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
      if (msg.url == DEFAULT_URL + "index.json") {
        msg.response_body.data = PROFILE_INDEX;
        index_requests++;
      } else if (msg.url == DEFAULT_URL + "123456.json") {
        msg.response_body.data = DEFAULT_PROFILE;
        profile_requests++;
      } else if (msg.url == DEFAULT_URL + "applies.json") {
        msg.response_body.data = DEFAULT_APPLIES;
        applies_requests++;
      }
      callback (this, msg);
    }
  }
}

public class ProfileCacheManager {
  private string[] profiles;
  private uint     counter;
  private string   applies;

  public ProfileCacheManager () {
    profiles = {};
    counter = 5;
  }

  public void flush () {
    profiles = {};
  }

  public void add_profile_from_data (string data) {
    profiles += data;

    if (loop != null) {
      counter--;
      if (counter < 1) {
        loop.quit ();
      }
    }
  }

  public void write_applies (string data) {
    applies = data;
  }

  public string[] get_profiles () {
    return profiles;
  }

  public string get_applies () {
    return applies;
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

  public void test_timeout () {
    loop = new MainLoop (null, false);

    var pcm = new ProfileCacheManager();
    var sm  = new SourceManager (pcm, DEFAULT_URL, 0);
    assert_nonnull (sm);

    Timeout.add (1000, () => {
      error ("Mainloop kept running - quitting test");
      loop.quit ();
      return false;
    });

    loop.run ();
    loop = null;

    var profiles = pcm.get_profiles ();
    assert (profiles.length == 1);
    assert (profiles[0] == DEFAULT_PROFILE);

    assert (index_requests > 1);
    assert (profile_requests > 1);
    assert (applies_requests > 1);
  }

  public static void setup () {
    index_requests = 0;
    profile_requests = 0;
    applies_requests = 0;
    loop = null;
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
    add_test ("timeout", pcm_suite, test_timeout);

    fc_suite.add_suite (pcm_suite);
    TestSuite.get_root ().add_suite (fc_suite);
    return Test.run();
  }
}
