namespace FleetCommander {
  public string dconf_dir;

  public delegate void TestFn ();

  public static void add_test (string name, TestSuite suite, TestFn fn) {
    suite.add(new TestCase(name, setup, (GLib.TestFixtureFunc)fn, teardown));
  }

  public static void setup () {
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
    rmrf (dconf_dir);
    dconf_dir = null;
  }

  public void test_construct () {
    var dp = new ConfigurationAdapterDconfProfiles (dconf_dir);
    assert_nonnull (dp);
  }

  public void test_simple_applies () {
    string content;

    //Profile Cache: There's no need to add profiles for this adaptor
    var pcache = new CacheData (); 
    //User index cache
    var icache = new CacheData ();
    assert_nonnull (icache);
    icache.set_applies ("applies-simple");
    var ui = new UserIndex(icache);
    assert_nonnull (ui);

    var dp = new ConfigurationAdapterDconfProfiles (dconf_dir);
    assert_nonnull (dp);

    dp.update (ui, pcache, 1000);
    assert (FileUtils.test (dconf_dir + "/1000", FileTest.EXISTS | FileTest.IS_REGULAR));

    FileUtils.get_contents (dconf_dir + "/1000", out content);
    assert ("user-db:user\nsystem-db:fleet-commander-simpleprofile" == content);
  }

  //TODO: Add test bootstrap

  public static int main (string[] args) {
    Test.init (ref args);
    var fc_suite = new TestSuite("fleetcommander");
    var ui_suite = new TestSuite("adapter-dconf-profiles");

    add_test ("construct", ui_suite, test_construct);
    add_test ("simple-applies", ui_suite, test_simple_applies);

    fc_suite.add_suite (ui_suite);
    TestSuite.get_root ().add_suite (fc_suite);
    return Test.run();
  }
}

