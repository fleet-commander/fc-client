namespace Posix {
  [CCode (cheader_filename = "fleet-commander-posix-helper.h", cname = "get_groups_for_user")]
  extern GLib.List<string> get_groups_for_user (uint32 user);
  [CCode (cheader_filename = "fleet-commander-posix-helper.h", cname = "uid_to_user_name")]
  extern string? uid_to_user_name (uint32 user);
}
