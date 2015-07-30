namespace FleetCommander {
  public string? cache_dir;
  public delegate void TestFn ();

  public static void add_test (string name, TestSuite suite, TestFn fn) {
    suite.add(new TestCase(name, setup, (GLib.TestFunc)fn, teardown));
  }

  /* Mocked Content Monitor */
  public class ContentMonitor : Object {
    public signal void content_updated ();
    public ContentMonitor (string path, uint interval = 1000) {
    }
  }

  /* setup and teardown */
  public static void setup () {
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

  /* Tests */
  public static void test_empty_cache () {
    var cd = new CacheData (cache_dir);
    assert_nonnull (cd);

    assert (cd.get_root () == null);
    assert (cd.get_path () == cache_dir + "/profiles.json");
  }

  public static void test_existing_cache () {
    var payload = "[{ \"description\" : \"\",
                      \"settings\" : {\"org.gnome.online-accounts\" : {}, \"org.gnome.gsettings\" : []},
                      \"applies-to\" : {\"users\" : [], \"groups\" : []},
                      \"name\" : \"My profile\",
                      \"etag\" : \"placeholder\",
                      \"uid\" : \"230637306661439565351338266313693940252\"}]";
    try {
      FileUtils.set_contents (cache_dir + "/profiles.json", payload);
    } catch (FileError e) {
      assert (false);
    }

    var cd = new CacheData (cache_dir);
    assert_nonnull (cd);

    assert_nonnull (cd.get_root ());
  }

  public static int main (string[] args) {
    Test.init (ref args);
    var fc_suite = new TestSuite("fleetcommander");
    var pcm_suite = new TestSuite("cache-data");

    add_test ("empty_cache", pcm_suite, test_empty_cache);
    add_test ("existing-cache", pcm_suite, test_existing_cache);
    //TODO: preexisting cache, bad JSON cache, removed existing cache

    fc_suite.add_suite (pcm_suite);
    TestSuite.get_root ().add_suite (fc_suite);
    return Test.run();
  }
}
