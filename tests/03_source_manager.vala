/* Mocked classes */
public string DEFAULT_URL;
public string PROFILE_INDEX;
public string DEFAULT_PROFILE;
public string DEFAULT_APPLIES;

public MainLoop? loop;
public uint index_requests;
public uint profile_requests;
public uint applies_requests;

public int INDEX_STATUS;
public int PROFILE_STATUS;
public int APPLIES_STATUS;

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

    public string get_path () {
      var parts = url.split ("/", 4);
      if (parts.length < 4)
        return url;

      return parts[3];
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
      if (msg.url == DEFAULT_URL + "index.json") {
        msg.response_body.data = PROFILE_INDEX;
        index_requests++;
        msg.status_code = INDEX_STATUS;
      } else if (msg.url == DEFAULT_URL + "123456.json") {
        msg.response_body.data = DEFAULT_PROFILE;
        profile_requests++;
        msg.status_code = PROFILE_STATUS;
      } else if (msg.url == DEFAULT_URL + "applies.json") {
        msg.response_body.data = DEFAULT_APPLIES;
        applies_requests++;
        msg.status_code = APPLIES_STATUS;
      }
      callback (this, msg);
    }
  }
}

public class ProfileCacheManager {
  private string[] profiles;
  private string   applies;
  uint counter;

  public ProfileCacheManager () {
    profiles = {};
    applies = "";
    counter = 4;
  }

  public void flush () {
    profiles = {};
  }

  public void write_profiles (Json.Object[] profiles_array) {
    profiles = {};
    var gen = new Json.Generator ();
    var node = new Json.Node (Json.NodeType.OBJECT);

    foreach (var p in profiles_array) {
      node.set_object (p);
      gen.set_root (node);
      profiles += gen.to_data (null);
      counter--;
    }

    var total = profile_requests + index_requests + applies_requests;
    //We make it 6 because it will run two iterations, one for the construct
    //and another one from the timeout
    if (loop != null && total == 6) {
      loop.quit ();
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

  public static void setup () {
    loop = null;
    index_requests = 0;
    profile_requests = 0;
    applies_requests = 0;

    DEFAULT_URL = "http://foobar/";
    PROFILE_INDEX = "[{\"url\": \"123456.json\"}]";
    DEFAULT_PROFILE = "{\"fake_profile_data-123456\":123}";
    DEFAULT_APPLIES = "{}";

    INDEX_STATUS = 200;
    PROFILE_STATUS = 200;
    APPLIES_STATUS = 200;
    loop = null;
  }

  public static void teardown () {
  }

  public void test_initialization () {
    var pcm = new ProfileCacheManager();
    var sm  = new SourceManager (pcm, DEFAULT_URL, 0);
    assert_nonnull (sm);

    assert (index_requests == 1);
    assert (profile_requests == 1);
    assert (applies_requests == 1);

    var profiles = pcm.get_profiles ();
    assert (profiles.length == 1);
    assert (profiles[0] == DEFAULT_PROFILE);
    assert (pcm.get_applies () == DEFAULT_APPLIES);
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
    assert (pcm.get_applies () == DEFAULT_APPLIES);

    assert (index_requests > 1);
    assert (profile_requests > 1);
    assert (applies_requests > 1);
  }

  public void test_inconsistent_responses_profile () {
    PROFILE_STATUS = 400;

    FcTest.expect_message (null, LogLevelFlags.LEVEL_WARNING, "*ERROR Message *");

    var pcm = new ProfileCacheManager();
    var sm  = new SourceManager (pcm, DEFAULT_URL, 0);

    assert (pcm.get_profiles ().length == 0);
    assert (pcm.get_applies () == "");
  }

  public void test_inconsistent_responses_applies () {
    APPLIES_STATUS = 400;

    FcTest.expect_message (null, LogLevelFlags.LEVEL_WARNING, "*ERROR Message *");

    var pcm = new ProfileCacheManager();
    var sm  = new SourceManager (pcm, DEFAULT_URL, 0);

    assert (pcm.get_profiles ().length == 0);
    assert (pcm.get_applies () == "");
  }

  public void test_inconsistent_responses_index () {
    INDEX_STATUS = 400;

    FcTest.expect_message (null, LogLevelFlags.LEVEL_WARNING, "*ERROR Message *");

    var pcm = new ProfileCacheManager();
    var sm  = new SourceManager (pcm, DEFAULT_URL, 0);

    assert (pcm.get_profiles ().length == 0);
    assert (pcm.get_applies () == "");
  }

  public void test_cache_preserved_on_inconsistent_session () {
    var pcm = new ProfileCacheManager();
    var sm  = new SourceManager (pcm, DEFAULT_URL, 0);

    INDEX_STATUS = 400;

    loop = new MainLoop (null, false);

    Timeout.add (1000, () => {
      error ("Mainloop kept running - quitting test");
      loop.quit ();
      assert(false);
      return false;
    });

    var profiles = pcm.get_profiles ();
    assert (profiles.length == 1);
    assert (profiles[0] == DEFAULT_PROFILE);
    assert (pcm.get_applies () == DEFAULT_APPLIES);
  }

  public static void add_test (string name, TestSuite suite, TestFn fn) {
    suite.add(new TestCase(name, setup, (GLib.TestFixtureFunc)fn, teardown));
  }

  public static int main (string[] args) {
    Test.init (ref args);
    var fc_suite = new TestSuite("fleetcommander");
    var pcm_suite = new TestSuite("source-manager");

    add_test ("initialization", pcm_suite, test_initialization);
    add_test ("timeout", pcm_suite, test_timeout);
    add_test ("inconsistent-profile", pcm_suite, test_inconsistent_responses_profile);
    add_test ("inconsistent-applies", pcm_suite, test_inconsistent_responses_applies);
    add_test ("inconsistent-index", pcm_suite, test_inconsistent_responses_index);
    add_test ("preserve-cache-after-inconsistent", pcm_suite, test_cache_preserved_on_inconsistent_session);

    fc_suite.add_suite (pcm_suite);
    TestSuite.get_root ().add_suite (fc_suite);
    return Test.run();
  }
}
