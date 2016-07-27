namespace FleetCommander {
  public string?         cache_dir;
  public MainLoop?       loop;
  public ContentMonitor? monitor_singleton;

  public const string PAYLOAD = "[{ \"description\" : \"\",
                      \"settings\" : {\"org.gnome.online-accounts\" : {}, \"org.gnome.gsettings\" : []},
                      \"applies-to\" : {\"users\" : [], \"groups\" : []},
                      \"name\" : \"My profile\",
                      \"etag\" : \"placeholder\",
                      \"uid\" : \"230637306661439565351338266313693940252\"},
                      {\"description\" : \"\",
                      \"settings\" : {\"org.gnome.online-accounts\" : {}, \"org.gnome.gsettings\" : []},
                      \"applies-to\" : {\"users\" : [], \"groups\" : []},
                      \"name\" : \"My other profile\",
                      \"etag\" : \"placeholder\",
                      \"uid\" : \"351338266313693940252230637306661439565\"}]";

  public delegate void TestFn ();

  public static void add_test (string name, TestSuite suite, TestFn fn) {
    suite.add(new TestCase(name, setup, (GLib.TestFixtureFunc)fn, teardown));
  }

  /* Mocked Content Monitor */
  public class ContentMonitor : Object {
    public signal void content_updated ();
    public ContentMonitor (string path, uint interval = 1000) {
      monitor_singleton = this;
    }
  }

  /* setup and teardown */
  public static void setup () {
    loop = null;
    monitor_singleton = null;

    try {
      cache_dir = DirUtils.make_tmp ("fleet_commander.test.XXXXXX");
    } catch (FileError e) {
      error ("Could not create temporary dir %s", e.message);
    }
    assert_nonnull (cache_dir);
  }

  public static void teardown () {
    if (cache_dir == null)
      return;

    FileUtils.remove (cache_dir + "/profiles.json");
    DirUtils.remove (cache_dir);
    cache_dir = null;
  }

  /* Utils */
  public static void write_content (string file, string content) {
    try {
      FileUtils.set_contents (file, content);
    } catch (FileError e) {
      error ("Could not write test data in the cache file %s", e.message);
    }
  }

  /* Tests */
  public static void test_no_cache_file () {
    var cd = new CacheData (cache_dir + "/profiles.json");
    assert_nonnull (cd);

    assert (cd.get_root () == null);
    assert (cd.get_path () == cache_dir + "/profiles.json");
  }

  public static void test_existing_cache () {
    write_content (cache_dir + "/profiles.json", PAYLOAD);

    var cd = new CacheData (cache_dir + "/profiles.json");
    assert_nonnull (cd);
    assert_nonnull (cd.get_root ());
  }

  public static void test_empty_cache_file () {
    write_content (cache_dir + "/profiles.json", "");

    FcTest.expect_message (null, LogLevelFlags.LEVEL_WARNING, "*Root JSON element*empty*");
    var cd = new CacheData (cache_dir + "/profiles.json");
    assert_nonnull (cd);
    assert (cd.get_root () == null);
  }

  public static void test_wrong_json_cache_file () {
    write_content (cache_dir + "/profiles.json", "{}");

    FcTest.expect_message (null, LogLevelFlags.LEVEL_WARNING, "*JSON element*not an array*");
    var cd = new CacheData (cache_dir  + "/profiles.json");
    assert_nonnull (cd);
    assert (cd.get_root () == null);
  }

  public static void test_dirty_cache_file () {
    write_content (cache_dir + "/profiles.json", "#@$@#W!*");

    FcTest.expect_message (null, LogLevelFlags.LEVEL_WARNING, "*error parsing*");
    var cd = new CacheData (cache_dir  + "/profiles.json");
    assert_nonnull (cd);
    assert (cd.get_root () == null);
  }

  public static void test_content_change () {
    bool parsed_called = false;
    write_content (cache_dir + "/profiles.json", PAYLOAD);
    loop = new MainLoop (null, false);

    var cd = new CacheData (cache_dir  + "/profiles.json");
    assert_nonnull (cd);
    assert_nonnull (cd.get_root ());

    FileUtils.remove (cache_dir + "/profiles.json");

    Idle.add (() => {
      assert_nonnull (monitor_singleton);
      //simulate a file monitor signal
      monitor_singleton.content_updated ();
      loop.quit ();
      loop = null;
      return false;
    });

    loop.run();
    assert (cd.get_root () == null);
  }

  public static void test_get_profile () {    
    write_content (cache_dir + "/profiles.json", PAYLOAD);

    var cd = new CacheData (cache_dir + "/profiles.json");
    assert_nonnull (cd);
    assert_nonnull (cd.get_root ());

    var prof = cd.get_profile ("230637306661439565351338266313693940252");
    assert_nonnull (cd);

    assert (prof.has_member ("uid"));
    assert (prof.get_string_member ("uid") == "230637306661439565351338266313693940252");

    prof = cd.get_profile ("FAKEUID");
    assert_null (cd);

    var profiles = cd.get_profiles ({"230637306661439565351338266313693940252", "351338266313693940252230637306661439565"});
    assert (profiles.length == 2);
    assert (profiles[0].get_string_member ("uid") == "230637306661439565351338266313693940252");
    assert (profiles[1].get_string_member ("uid") == "351338266313693940252230637306661439565");
    assert (profiles[0].get_string_member ("name") == "My profile");
    assert (profiles[1].get_string_member ("name") == "My other profile");
  }

  public static int main (string[] args) {
    Test.init (ref args);
    var fc_suite = new TestSuite("fleetcommander");
    var pcm_suite = new TestSuite("cache-data");

    add_test ("no-cache", pcm_suite, test_no_cache_file);
    add_test ("existing-cache", pcm_suite, test_existing_cache);
    add_test ("empty-cache-file", pcm_suite, test_empty_cache_file);
    add_test ("dirty-cache", pcm_suite, test_dirty_cache_file);
    add_test ("content-change", pcm_suite, test_content_change);
    add_test ("get-profile", pcm_suite, test_get_profile);
    //TODO: removed existing cache

    fc_suite.add_suite (pcm_suite);
    TestSuite.get_root ().add_suite (fc_suite);
    return Test.run();
  }
}
