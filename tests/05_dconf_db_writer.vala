namespace FleetCommander {
  public string?         dconf_dir;
  public CacheData? cache_data_singleton = null;
  public delegate void TestFn ();

  //Mock CacheData
  public class CacheData {
    private string cache_path;
    public signal void parsed();
    public CacheData (string cache_path) {
      this.cache_path = cache_path;
      cache_data_singleton = this;
    }

    public string get_path () {
      return cache_path;
    }

    public Json.Node? get_root () {
      return null;
    }
  }

  public static void add_test (string name, TestSuite suite, TestFn fn) {
    suite.add(new TestCase(name, setup, (GLib.TestFunc)fn, teardown));
  }

  public static void setup () {
    cache_data_singleton = null;
    try {
      dconf_dir = DirUtils.make_tmp ("fleet_commander.test.XXXXXX");
    } catch (FileError e) {
      error ("Could not create temporary dir %s", e.message);
    }

    assert_nonnull (dconf_dir);
  }

  public static void teardown () {
    if (dconf_dir == null)
      return;

    DirUtils.remove (dconf_dir);
    dconf_dir = null;
  }

  public static void test_construct () {
    var ddw = new DconfDbWriter (new CacheData ("/some/path"), dconf_dir);
    assert_nonnull (ddw);
  }

  public static int main (string[] args) {
    Test.init (ref args);
    var fc_suite = new TestSuite("fleetcommander");
    var ddw_suite = new TestSuite("dconf-db-writer");

    add_test ("construct", ddw_suite, test_construct);
    //TODO: commit_profile, call_dconf_update, update_databases

    fc_suite.add_suite (ddw_suite);
    TestSuite.get_root ().add_suite (fc_suite);
    return Test.run();
  }
}
