namespace FleetCommander {
  public delegate void TestFn ();

  public static void add_test (string name, TestSuite suite, TestFn fn) {
    suite.add(new TestCase(name, setup, (GLib.TestFunc)fn, teardown));
  }

  public static void setup () {
  }

  public static void teardown () {
  }

  public void test_construct () {
    var ui = new UserIndex(new CacheData ());
    assert_nonnull (ui);
  }

  public void test_simple_applies () {
    var cache = new CacheData ();
    cache.set_applies ("applies-simple");

    var ui = new UserIndex(cache);

    var up = ui.get_user_profiles ();
    var gp = ui.get_group_profiles ();

    assert_nonnull (up);
    assert_nonnull (gp);

    assert (up.has_member ("user1"));
    var profiles = up.get_array_member ("user1");
    assert_nonnull (profiles);
    assert (profiles.get_length () == 1);
    assert (profiles.get_string_element (0) != null);
    assert (profiles.get_string_element (0) == "simpleprofile");

    assert (gp.has_member ("group1"));
    profiles = gp.get_array_member ("group1");
    assert_nonnull (profiles);
    assert (profiles.get_length () == 1);
    assert (profiles.get_string_element (0) != null);
    assert (profiles.get_string_element (0) == "simpleprofile");
  }

  //TODO: test CacheData.parsed() signal

  public static int main (string[] args) {
    Test.init (ref args);
    var fc_suite = new TestSuite("fleetcommander");
    var ui_suite = new TestSuite("user-index");

    add_test ("construct", ui_suite, test_construct);
    add_test ("simple-applies", ui_suite, test_simple_applies);

    fc_suite.add_suite (ui_suite);
    TestSuite.get_root ().add_suite (fc_suite);
    return Test.run();
  }
}
