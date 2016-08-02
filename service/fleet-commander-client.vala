namespace FleetCommander {
    public static int main (string[] args) {
    var ml = new GLib.MainLoop (null, false);

    var config  = new ConfigReader();
    var profmgr = new ProfileCacheManager(config.cache_path);
    var srcmgr  = new SourceManager(profmgr, config.source, config.polling_interval);

    var profiles_cache = new CacheData(config.cache_path + "/profiles.json");
    var dconfdb = new DconfDbWriter(profiles_cache, config.dconf_db_path);
    var uindex  = new UserIndex(new CacheData (config.cache_path + "/applies.json"));

    var usermgr = new UserSessionHandler(uindex, profiles_cache);
    usermgr.register_handler (new ConfigurationAdapterDconfProfiles (config.dconf_profile_path));
    usermgr.register_handler (new ConfigurationAdapterGOA (config.goa_run_path));
    usermgr.register_handler (new ConfigurationAdapterNM ());

    ml.run();
    return 0;
  }
}
