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
    var pcache = new CacheData ();
    pcache.add_profile ("profile-simple");
    //User index cache
    var icache = new CacheData ();
    icache.set_applies ("applies-simple");
    var ui = new UserIndex(icache);
    var dp = new ConfigurationAdapterDconfProfiles (dconf_dir + "/dconf");

    dp.update (ui, pcache, 1000);
    check_profile_content (dp, ui, pcache, 1000,
                           "user-db:user\nsystem-db:fleet-commander-simpleprofile");
  }

  //TODO: Add test bootstrap
  public void test_delition_after_creation () {
    string content;

    var pcache = new CacheData ();
    pcache.add_profile ("profile-simple");
    var icache = new CacheData ();
    icache.set_applies ("applies-simple");
    var ui = new UserIndex(icache);
    var dp = new ConfigurationAdapterDconfProfiles (dconf_dir + "/dconf");;

    dp.update (ui, pcache, 1000);
    check_profile_content (dp, ui, pcache, 1000,
                           "user-db:user\nsystem-db:fleet-commander-simpleprofile");

    pcache.profiles = null;
    icache.applies = null;

    dp.update (ui, pcache, 1000);
    assert (FileUtils.test (dconf_dir + "/dconf/1000", FileTest.EXISTS) == false);
  }

  public void test_redundant_applies_user_group () {
    var pcache = new CacheData ();
    pcache.add_profile ("profile-simple");
    pcache.add_profile ("profile-moar");
    var icache = new CacheData ();
    icache.set_applies ("applies-duplicates");
    var ui = new UserIndex(icache);
    var dp = new ConfigurationAdapterDconfProfiles (dconf_dir + "/dconf");

    dp.update (ui, pcache, 1000);
    check_profile_content (dp, ui, pcache, 1000,
                           "user-db:user\nsystem-db:fleet-commander-moarprofile\nsystem-db:fleet-commander-simpleprofile");
  }

  public void test_non_dconf_profile () {
    var pcache = new CacheData ();
    pcache.add_profile ("profile-goa");
    pcache.add_profile ("profile-dumb");
    var icache = new CacheData ();
    icache.set_applies ("applies-goa");
    var ui = new UserIndex(icache);
    var dp = new ConfigurationAdapterDconfProfiles (dconf_dir + "/dconf");

    dp.update (ui, pcache, 1000);

    assert (FileUtils.test (dconf_dir + "/dconf/1000", FileTest.EXISTS) == false);
  }

  public void test_just_libreoffice_profile () {
    var pcache = new CacheData ();
    pcache.add_profile ("profile-libreoffice");
    var icache = new CacheData ();
    icache.set_applies ("applies-libreoffice");
    var ui = new UserIndex(icache);
    var dp = new ConfigurationAdapterDconfProfiles (dconf_dir + "/dconf");

    dp.update (ui, pcache, 1000);
    check_profile_content (dp, ui, pcache, 1000, "user-db:user\nsystem-db:fleet-commander-libreoffice");
  }

  void check_profile_content (ConfigurationAdapterDconfProfiles dp,
                              UserIndex ui, CacheData pcache, uint32 uid, string result) {
    string content;

    dp.update (ui, pcache, uid);
    var path = "%s/dconf/%u".printf(dconf_dir, uid);
    assert (FileUtils.test (path, FileTest.EXISTS | FileTest.IS_REGULAR));

    FileUtils.get_contents (dconf_dir + "/dconf/1000", out content);
    assert (result == content);
  }

  public static int main (string[] args) {
    Test.init (ref args);
    var fc_suite = new TestSuite("fleetcommander");
    var ui_suite = new TestSuite("adapter-dconf-profiles");

    add_test ("construct", ui_suite, test_construct);
    add_test ("simple-applies", ui_suite, test_simple_applies);
    add_test ("deletion-after-creation", ui_suite, test_delition_after_creation);
    add_test ("redundant-applies-user-group", ui_suite, test_redundant_applies_user_group);
    add_test ("non-dconf-profile", ui_suite, test_non_dconf_profile);
    add_test ("test_just_libreoffice", ui_suite, test_just_libreoffice_profile);

    fc_suite.add_suite (ui_suite);
    TestSuite.get_root ().add_suite (fc_suite);
    return Test.run();
  }
}

