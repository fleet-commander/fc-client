namespace FleetCommander {
  internal string? cache_dir = null;

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
    FileUtils.remove (cache_dir + "/applies.json");
    DirUtils.remove (cache_dir);
    cache_dir = null;
  }

  public static Json.Node? get_json_root (string filename) {
    var path = string.join ("/", cache_dir, filename);
    if (FileUtils.test (path, FileTest.EXISTS) == false)
      return null;
    if (FileUtils.test (path, FileTest.IS_REGULAR) == false)
      return null;

    var parser = new Json.Parser ();
    parser.load_from_file (path);
    return parser.get_root ();
  }

  public static Json.Node? get_profiles_root () {
    return get_json_root ("profiles.json");
  }

  public static Json.Node? get_applies_root () {
    return get_json_root ("applies.json");
  }

  public static void test_add_profile () {
    var pcm = new ProfileCacheManager (cache_dir);

    var prof1 = new Json.Object ();
    prof1.set_string_member ("foo", "bar");
    var prof2 = new Json.Object ();
    prof2.set_string_member ("baz", "foo");

    pcm.write_profiles ({prof1, prof2});

    var root = get_profiles_root();
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

  public static void test_write_applies () {
    var pcm = new ProfileCacheManager (cache_dir);
    pcm.write_applies ("{}");

    assert (get_applies_root () != null);
  }

  public static void test_flush () {
    var pcm = new ProfileCacheManager (cache_dir);

    var prof = new Json.Object ();
    prof.set_string_member ("foo", "bar");
    pcm.write_profiles ({prof});
    pcm.write_applies ("{}");
    pcm.flush ();

    assert (get_profiles_root () == null);
    assert (get_applies_root () == null);
  }

  public static void test_empty_cachedir () {
    var pcm = new ProfileCacheManager (cache_dir);

    assert (get_profiles_root () == null);
    assert (get_applies_root () == null);
  }

  public static void test_invalid_path () {
    var pcm = new ProfileCacheManager ("/foo/bar/baz/dir/random");
    assert (get_profiles_root () == null);

    FcTest.expect_message (null, LogLevelFlags.LEVEL_WARNING, "*Could not write*");

    var prof = new Json.Object ();
    prof.set_string_member ("foo", "bar");
    pcm.write_profiles ({prof});

    assert (get_profiles_root () == null);

    FcTest.expect_message (null, LogLevelFlags.LEVEL_WARNING, "*Could not rewrite*");
    pcm.write_applies ("{}");
    assert (get_applies_root () == null);

    pcm.flush ();
    assert (get_profiles_root () == null);
    assert (get_applies_root () == null);
  }

  public delegate void TestFn ();

  public static void add_test (string name, TestSuite suite, TestFn fn) {
    suite.add(new TestCase(name, setup, (GLib.TestFixtureFunc)fn, teardown));
  }

  public static int main (string[] args) {
    Test.init (ref args);
    var fc_suite = new TestSuite("fleetcommander");
    var pcm_suite = new TestSuite("profile-cache-manager");

    add_test ("add-profile", pcm_suite, test_add_profile);
    add_test ("write-applies", pcm_suite, test_write_applies);
    add_test ("flush", pcm_suite, test_flush);
    add_test ("empty-cache-dir", pcm_suite, test_empty_cachedir);
    add_test ("invalid-path", pcm_suite, test_invalid_path);

    fc_suite.add_suite (pcm_suite);
    TestSuite.get_root ().add_suite (fc_suite);
    return Test.run();
  }
}
