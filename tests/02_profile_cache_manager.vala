namespace FleetCommander {
  internal string? cache_dir = null;

  public static void setup () {
    cache_dir = DirUtils.make_tmp ("fleet_commander.test.XXXXXX");
    assert_nonnull (cache_dir);
  }

  public static void teardown () {
    if (cache_dir == null)
      return;

    FileUtils.remove (cache_dir + "/profiles.json");
    DirUtils.remove (cache_dir);
    cache_dir = null;
  }

  public static void test_add_profile () {
    var pcm = new ProfileCacheManager (cache_dir);

    pcm.add_profile_from_data ("{\"foo\": \"bar\"}");
    pcm.add_profile_from_data ("{\"baz\": \"foo\"}");

    var root = pcm.get_cache_root();
    assert_nonnull (root);
    assert_nonnull (root.get_array ());
    assert (root.get_array ().get_length () == 2);

    var profile_node = root.get_array ().get_element (0);
    assert_nonnull (profile_node);
    var profile_object = profile_node.get_object ();
    assert_nonnull (profile_object);
    assert (profile_object.has_member ("foo"));
    assert_nonnull (profile_object.get_string_member ("foo"));
    assert (profile_object.get_string_member ("foo") == "bar");

    profile_node = root.get_array ().get_element (1);
    assert_nonnull (profile_node);
    profile_object = profile_node.get_object ();
    assert_nonnull (profile_object);
    assert (profile_object.has_member ("baz"));
    assert_nonnull (profile_object.get_string_member ("baz"));
    assert (profile_object.get_string_member ("baz") == "foo");
  }

  public static void test_flush () {
    var pcm = new ProfileCacheManager (cache_dir);

    pcm.add_profile_from_data ("{\"foo\": \"bar\"}");
    pcm.flush ();

    var root = pcm.get_cache_root ();
    assert_nonnull (root);
    assert_nonnull (root.get_array ());
    assert (root.get_array ().get_length () == 0);
  }

  public delegate void TestFn ();

  public static void add_test (string name, TestSuite suite, TestFn fn) {
    suite.add(new TestCase(name, setup, (GLib.TestFunc)fn, teardown));
  }

  public static int main (string[] args) {
    Test.init (ref args);
    var fc_suite = new TestSuite("fleetcommander");
    var pcm_suite = new TestSuite("profile-cache-manager");

    add_test ("add-profile", pcm_suite, test_add_profile);
    add_test ("flush", pcm_suite, test_flush);

    fc_suite.add_suite (pcm_suite);
    TestSuite.get_root ().add_suite (fc_suite);
    return Test.run();
  }
}
