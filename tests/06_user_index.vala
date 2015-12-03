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

  public void test_rebuild_index_empty () {
    var ui = new UserIndex(new CacheData ());
    assert_nonnull (ui);
  }

  public static int main (string[] args) {
    Test.init (ref args);
    var fc_suite = new TestSuite("fleetcommander");
    var ui_suite = new TestSuite("user-index");

    add_test ("construct", ui_suite, test_construct);

    fc_suite.add_suite (ui_suite);
    TestSuite.get_root ().add_suite (fc_suite);
    return Test.run();
  }
}
