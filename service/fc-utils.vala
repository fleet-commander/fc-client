namespace FleetCommander {
  public static bool dconf_db_can_write (string dconf_db_path) {
    if (FileUtils.test (dconf_db_path, FileTest.EXISTS) == false) {
      warning ("dconf datbase path %s does not exists", dconf_db_path);
      return false;
    }
    if (Posix.access(dconf_db_path, Posix.W_OK | Posix.X_OK) != 0) {
      warning ("Cannot write data onto %s", dconf_db_path);
      return false;
    }
    return true;
  }

  public static bool object_has_members (Json.Object object, string[] keys) {
    bool all = true;
    var members = object.get_members();
    foreach (var key in keys) {
      all = members.find_custom (key, (a,b) => { return (a == b)? 0 : 1; }) != null;
      if (!all) {
        debug ("key %s not found", key);
        break;
      }
    }
    return all;
  }
}
