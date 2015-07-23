namespace FleetCommander {
  public static int main (string[] args) {
    var ml = new GLib.MainLoop (null, false);

    var config  = new ConfigReader();
    var profmgr = new ProfileCacheManager(config.cache_path);
    var srcmgr  = new SourceManager(profmgr, config.source, config.polling_interval);

    var cache   = new CacheData(config.cache_path);
    var dconfdb = new DconfDbWriter(cache, config.dconf_db_path);
    var uindex  = new UserIndex(cache);
    var usermgr = new UserSessionHandler(uindex, config.dconf_db_path, config.dconf_profile_path);

    ml.run();
    return 0;
  }
}
