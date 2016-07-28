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

  public static void rmrf (string path) throws Error {
    rmrf_file (File.new_for_path (path));
  }

  private static void rmrf_file (File file) throws Error {
    if (!file.query_exists (null))
      return;

    var info = file.query_info ("standard::*", FileQueryInfoFlags.NONE);
    if (info.get_file_type () == FileType.DIRECTORY) {
      var enumerator = file.enumerate_children ("standard::*", FileQueryInfoFlags.NONE, null);
      for (var child = enumerator.next_file (); child != null; child = enumerator.next_file ()) {
        var child_file = enumerator.get_child (child);
        if (child.get_file_type () == FileType.DIRECTORY)
          rmrf_file (child_file);
        else
          child_file.delete ();
      }
    }
    file.delete (null);
  }
}
