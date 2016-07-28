namespace FleetCommander {
  public string goa_dir;

  public delegate void TestFn ();

  public static void add_test (string name, TestSuite suite, TestFn fn) {
    suite.add(new TestCase(name, setup, (GLib.TestFixtureFunc)fn, teardown));
  }

  public static void setup () {
    try {
      goa_dir = DirUtils.make_tmp ("fleet_commander.test.XXXXXX");
    } catch (FileError e) {
      error ("Could not create temporary dir %s", e.message);
    }

    assert_nonnull (goa_dir);
  }

  public static void teardown () {
    if (goa_dir == null)
      return;
    rmrf (goa_dir);
    goa_dir = null;
  }

  public void test_construct () {
    var dp = new ConfigurationAdapterGOA (goa_dir + "/goa");
    assert_nonnull (dp);
  }

  public void test_basic_case () {
    var pcache = new CacheData ();
    var icache = new CacheData ();
    icache.set_applies ("applies-goa");
    pcache.add_profile ("profile-goa");
    var ui = new UserIndex(icache);

    var dp = new ConfigurationAdapterGOA (goa_dir + "/goa");
    assert_nonnull (dp);

    dp.update (ui, pcache, 1000);
    var path = "%s/%u/fleet-commander-accounts.conf".printf (goa_dir + "/goa", 1000);
    assert (FileUtils.test (path, FileTest.EXISTS));

    string content;
    FileUtils.get_contents (path, out content);

    var keyfile = new KeyFile ();
    keyfile.set_string ("Account account_fc_FooBar", "Provider", "foobar");
    keyfile.set_string ("Account account_fc_BarBaz", "Provider", "barbaz");

    assert (content == keyfile.to_data ());

    icache = new CacheData ();
    pcache = new CacheData ();
    icache.set_applies ("applies-empty");
    ui = new UserIndex (icache);

    dp.update (ui, pcache, 1000);

    assert (FileUtils.test (path, FileTest.EXISTS) == false);
  }

  public static int main (string[] args) {
    Test.init (ref args);
    var fc_suite = new TestSuite("fleetcommander");
    var ui_suite = new TestSuite("adapter-goa");

    add_test ("construct", ui_suite, test_construct);
    add_test ("basic-case", ui_suite, test_basic_case);

    fc_suite.add_suite (ui_suite);
    TestSuite.get_root ().add_suite (fc_suite);
    return Test.run();
  }
}
