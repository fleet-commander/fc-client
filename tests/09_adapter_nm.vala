namespace FleetCommander.NMTest {
  public delegate void TestFn ();

  public static void add_test (string name, TestSuite suite, TestFn fn) {
    suite.add(new TestCase(name, setup, (GLib.TestFixtureFunc)fn, teardown));
  }

  public static void setup () {
  }

  public static void teardown () {
  }

  public void test_construct () {
    var dp = new ConfigurationAdapterNM (new NetworkManager.SettingsHelper ());
    assert_nonnull (dp);
  }

  public void test_ethprofile () {
    var pcache = new CacheData ();
    var icache = new CacheData ();
    icache.set_applies ("applies-nm-all");
    pcache.add_profile ("profile-nm-all");
    var ui = new UserIndex (icache);

    var nmsh = new NetworkManager.SettingsHelper ();
    nmsh.emit_bus_appeared ();
    var nm = new FleetCommander.ConfigurationAdapterNM (nmsh);

    //applies to group1
    nm.update (ui, pcache, 1000);

/*    assert (nmsh.added_connections.lookup ("37fd27fb-8e3b-4e08-9bd4-b68469879e79") != null);
    assert (nmsh.added_connections.lookup ("5c66fffa-baf6-40ce-b04d-b2575bf3f6a0") != null);
    assert (nmsh.added_connections.lookup ("e04f8eda-d1d9-4d8b-811b-49ae463600de") != null);*/
    assert (nmsh.added_connections.size () == 3);

    nmsh = new NetworkManager.SettingsHelper ();
    nmsh.emit_bus_appeared ();
    nm =  new FleetCommander.ConfigurationAdapterNM (nmsh);

    nmsh.add_present_uuid ("37fd27fb-8e3b-4e08-9bd4-b68469879e79");
    nmsh.add_present_uuid ("5c66fffa-baf6-40ce-b04d-b2575bf3f6a0");
    nmsh.add_present_uuid ("e04f8eda-d1d9-4d8b-811b-49ae463600de");

    nm.update (ui, pcache, 1000);

    assert (nmsh.updated_connections.size () == 3);
  }

  //TODO: Test if it doesn't do anytihng when we're not in bus

  public static int main (string[] args) {
    Test.init (ref args);
    var fc_suite = new TestSuite("fleetcommander");
    var nm_suite = new TestSuite("adapter-network-manager");

    add_test ("construct", nm_suite, test_construct);
    add_test ("profile-all", nm_suite, test_ethprofile);

    //Test with existing connection

    fc_suite.add_suite (nm_suite);
    TestSuite.get_root ().add_suite (fc_suite);
    return Test.run();
  }
}
