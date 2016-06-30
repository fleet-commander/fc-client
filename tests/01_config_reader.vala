namespace FleetCommander {
  public delegate void TestFn ();
  public static void add_test (string name, TestSuite suite, TestFn fn) {
    suite.add(new TestCase(name, setup, (GLib.TestFixtureFunc)fn, teardown));
  }

  public string config_dir;

  public static void setup () {
    config_dir = Environment.get_variable ("FC_PROFILES_PATH");
    assert (config_dir != null);
  }

  public static void teardown () {
  }

  public static void assert_default_values (ConfigReader cr) {
    assert (cr.source == "");
    assert (cr.polling_interval == 60 * 60);
    assert (cr.cache_path == "/var/cache/fleet-commander");
    assert (cr.dconf_db_path == "/etc/dconf/db");
    assert (cr.dconf_profile_path == "/run/dconf/user");
  }

  public static void test_defaults () {
    FcTest.expect_message (null, LogLevelFlags.LEVEL_WARNING, "*Error parsing configuration file*");
    var cr = new ConfigReader (string.join("/", config_dir, "does-not-exists.conf"));
    assert_default_values (cr);
  }

  public static void test_read () {
    var cr = new ConfigReader (string.join("/", config_dir, "simple-test.conf"));

    assert (cr.source == "http://somehost:99/somepath/");
    assert (cr.polling_interval == 40);
    assert (cr.cache_path == "/foo/cache");
    assert (cr.dconf_db_path == "/foo/dconf/db");
    assert (cr.dconf_profile_path == "/foo/dconf/profile");
  }

  public static void test_zero () {
    FcTest.expect_message (null, LogLevelFlags.LEVEL_WARNING, "*Value 0 is invalid*");
    var cr = new ConfigReader (string.join("/", config_dir, "interval-zero.conf"));

    assert (cr.polling_interval == 60 * 60);
  }

  public static void test_missing_group () {
    FcTest.expect_message (null, LogLevelFlags.LEVEL_WARNING, "*[fleet-commander] group*");
    var cr = new ConfigReader (string.join("/", config_dir, "missing-group.conf"));
    assert_default_values (cr);
  }

  public static int main (string[] args) {
    Test.init (ref args);
    var fc_suite = new TestSuite("fleetcommander");
    var cr_suite = new TestSuite("config-reader");

    add_test ("defaults", cr_suite, test_defaults);
    add_test ("read", cr_suite, test_read);
    add_test ("interval-zero", cr_suite, test_zero);
    add_test ("missing-group", cr_suite, test_missing_group);

    fc_suite.add_suite (cr_suite);
    TestSuite.get_root ().add_suite (fc_suite);
    return Test.run();
  }
}
