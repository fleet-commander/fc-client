namespace Posix {
  GLib.List<string> get_groups_for_user (uint32 user) {
    List<string> groups = null;
    groups.append ("group1");
    groups.append ("group2");
    return groups;
  }
  string? uid_to_user_name (uint32 user) {
    return "user1";
  }
}
