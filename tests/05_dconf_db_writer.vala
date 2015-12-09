namespace FleetCommander {
  public string?       dconf_dir;

  public delegate void TestFn ();

  public static void add_test (string name, TestSuite suite, TestFn fn) {
    suite.add(new TestCase(name, setup, (GLib.TestFunc)fn, teardown));
  }

  public static void setup () {
    try {
      dconf_dir = DirUtils.make_tmp ("fleet_commander.test.XXXXXX");
    } catch (FileError e) {
      error ("Could not create temporary dir %s", e.message);
    }

    assert_nonnull (dconf_dir);
    Environment.set_variable ("FC_TEST_TMPDIR", dconf_dir, true);
  }

  public static void teardown () {
    if (dconf_dir == null)
      return;

    FileUtils.remove (dconf_dir + "/fleet-commander-libreoffice.d/generated");
    DirUtils.remove (dconf_dir + "/fleet-commander-libreoffice.d");
    FileUtils.remove (dconf_dir + "/fleet-commander-libreoffice");

    FileUtils.remove (dconf_dir + "/fleet-commander-gsettings.d/generated");
    DirUtils.remove (dconf_dir + "/fleet-commander-gsettings.d");
    FileUtils.remove (dconf_dir + "/fleet-commander-gsettings");

    FileUtils.remove (dconf_dir + "/fleet-commander-mixed.d/generated");
    DirUtils.remove (dconf_dir + "/fleet-commander-mixed.d");
    FileUtils.remove (dconf_dir + "/fleet-commander-mixed");

    DirUtils.remove (dconf_dir);

    dconf_dir = null;
    Environment.unset_variable ("FC_TEST_TMPDIR");
  }

  public static void test_construct () {
    var ddw = new DconfDbWriter (new CacheData (), dconf_dir);
    assert_nonnull (ddw);
  }

  public static void test_update_empty_profile () {
    var cachedata = new CacheData ();
    cachedata.add_profile ("profile-empty");

    var ddw = new DconfDbWriter (cachedata, dconf_dir);

    cachedata.parsed ();

    assert (FileUtils.test (dconf_dir + "/fleet-commander-empty.d/generated", FileTest.EXISTS) == false);
    assert (FileUtils.test (dconf_dir + "/fleet-commander-empty.d", FileTest.EXISTS) == false);
    assert (FileUtils.test (dconf_dir + "/fleet-commander-empty", FileTest.EXISTS) == false);
  }

  public static void test_update_libreoffice_profile () {
    var cachedata = new CacheData ();
    cachedata.add_profile ("profile-libreoffice");

    var ddw = new DconfDbWriter (cachedata, dconf_dir);
    cachedata.parsed ();

    var generated_path = dconf_dir + "/fleet-commander-libreoffice.d/generated";

    assert (FileUtils.test (generated_path, FileTest.EXISTS));
    assert (FileUtils.test (generated_path, FileTest.IS_REGULAR));
    assert (FileUtils.test (dconf_dir + "/fleet-commander-libreoffice.d", FileTest.EXISTS));
    assert (FileUtils.test (dconf_dir + "/fleet-commander-libreoffice", FileTest.IS_REGULAR));

    var keyfile = new KeyFile();
    keyfile.load_from_file (generated_path, KeyFileFlags.NONE);

    assert (keyfile.get_groups ().length == 1);
    assert (keyfile.has_group ("org/libreoffice/registry/foo/bar"));
    assert (keyfile.has_key ("org/libreoffice/registry/foo/bar", "bool"));
    assert (keyfile.get_value ("org/libreoffice/registry/foo/bar", "bool") == "false");
  }

  public static void test_update_gsettings_profile () {
    var cachedata = new CacheData ();
    cachedata.add_profile ("profile-gsettings");

    var ddw = new DconfDbWriter (cachedata, dconf_dir);
    cachedata.parsed ();

    var generated_path = dconf_dir + "/fleet-commander-gsettings.d/generated";

    assert (FileUtils.test (generated_path, FileTest.EXISTS));
    assert (FileUtils.test (generated_path, FileTest.IS_REGULAR));
    assert (FileUtils.test (dconf_dir + "/fleet-commander-gsettings.d", FileTest.EXISTS));
    assert (FileUtils.test (dconf_dir + "/fleet-commander-gsettings", FileTest.EXISTS));
    assert (FileUtils.test (dconf_dir + "/fleet-commander-gsettings", FileTest.IS_REGULAR));

    var keyfile = new KeyFile();
    keyfile.load_from_file (generated_path, KeyFileFlags.NONE);

    assert (keyfile.get_groups ().length == 1);
    assert (keyfile.has_group ("org/gnome/foo/bar"));
    assert (keyfile.has_key ("org/gnome/foo/bar", "bool"));
    assert (keyfile.get_value ("org/gnome/foo/bar", "bool") == "false");
  }

  public static void test_update_mixed_profile () {
    var cachedata = new CacheData ();
    cachedata.add_profile ("profile-mixed");

    var ddw = new DconfDbWriter (cachedata, dconf_dir);
    cachedata.parsed ();

    var generated_path = dconf_dir + "/fleet-commander-mixed.d/generated";

    assert (FileUtils.test (generated_path, FileTest.EXISTS));
    assert (FileUtils.test (generated_path, FileTest.IS_REGULAR));
    assert (FileUtils.test (dconf_dir + "/fleet-commander-mixed.d", FileTest.EXISTS));
    assert (FileUtils.test (dconf_dir + "/fleet-commander-mixed", FileTest.EXISTS));
    assert (FileUtils.test (dconf_dir + "/fleet-commander-mixed", FileTest.IS_REGULAR));

    var keyfile = new KeyFile();
    keyfile.load_from_file (generated_path, KeyFileFlags.NONE);

    assert (keyfile.get_groups ().length == 2);
    assert (keyfile.has_group ("org/libreoffice/registry/foo/bar"));
    assert (keyfile.has_key ("org/libreoffice/registry/foo/bar", "bool"));
    assert (keyfile.get_value ("org/libreoffice/registry/foo/bar", "bool") == "false");
    assert (keyfile.has_group ("org/gnome/foo/bar"));
    assert (keyfile.has_key ("org/gnome/foo/bar", "bool"));
    assert (keyfile.get_value ("org/gnome/foo/bar", "bool") == "false");
  }

  public static void test_update_existing_profile () {
    DirUtils.create (dconf_dir + "/fleet-commander-gsettings.d", 0x1E4);
    FileUtils.set_contents (dconf_dir + "/fleet-commander-gsettings", "");
    FileUtils.set_contents (dconf_dir + "/fleet-commander-gsettings.d/generated", "[b/c/d]\ne = 1");

    DirUtils.create (dconf_dir + "/fleet-commander-random.d", 0x1E4);
    FileUtils.set_contents (dconf_dir + "/fleet-commander-random", "");
    FileUtils.set_contents (dconf_dir + "/fleet-commander-random.d/generated", "[b/c/d]\nf = 3");

    var cachedata = new CacheData ();
    cachedata.add_profile ("profile-gsettings");
    var ddw = new DconfDbWriter (cachedata, dconf_dir);
    cachedata.parsed ();

    assert(FileUtils.test (dconf_dir + "/fleet-commander-random", FileTest.EXISTS) == false);
    assert(FileUtils.test (dconf_dir + "/fleet-commander-random.d", FileTest.EXISTS) == false);

    var keyfile = new KeyFile ();
    keyfile.load_from_file (dconf_dir + "/fleet-commander-gsettings.d/generated", KeyFileFlags.NONE);

    assert (keyfile.get_groups ().length == 1);
    assert (keyfile.has_group ("org/gnome/foo/bar"));
    assert (keyfile.has_key ("org/gnome/foo/bar", "bool"));
    assert (keyfile.get_value ("org/gnome/foo/bar", "bool") == "false");
  }

  public static int main (string[] args) {
    Test.init (ref args);
    var fc_suite = new TestSuite("fleetcommander");
    var ddw_suite = new TestSuite("dconf-db-writer");

    add_test ("construct", ddw_suite, test_construct);
    add_test ("update-empty-profile", ddw_suite, test_update_empty_profile);
    add_test ("update-libreoffice-profile", ddw_suite, test_update_libreoffice_profile);
    add_test ("update-gsettings-profile", ddw_suite, test_update_gsettings_profile);
    add_test ("update-mixed-profile", ddw_suite, test_update_mixed_profile);
    add_test ("update-existing-profile", ddw_suite, test_update_existing_profile);

    //TODO: implement and test locked keys

    fc_suite.add_suite (ddw_suite);
    TestSuite.get_root ().add_suite (fc_suite);
    return Test.run();
  }
}
