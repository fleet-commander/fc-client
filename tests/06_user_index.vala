namespace FleetCommander {
  public delegate void TestFn ();

  //MockCacheData
  //TODO: move this to a different file to share with DconfDbWriter
  public class CacheData {
    private string cache_path;
    public signal void parsed();
    public CacheData (string cache_path) {
      this.cache_path = cache_path;
    }

    public string get_path () {
      return cache_path;
    }

    public Json.Node? get_root () {
      var builder = new Json.Builder();
      builder.begin_array();
      builder.end_array();
      return builder.get_root();
    }
  }

  public static void add_test (string name, TestSuite suite, TestFn fn) {
    suite.add(new TestCase(name, setup, (GLib.TestFunc)fn, teardown));
  }

  public static void setup () {
  }

  public static void teardown () {
  }

  public void test_construct () {
    var ui = new UserIndex(new CacheData ("/some/path"));
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
