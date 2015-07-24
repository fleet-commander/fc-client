namespace FcTest {
  [CCode (cheader_filename = "glib.h", cname = "g_test_expect_message")]
  extern void expect_message (string? domain, GLib.LogLevelFlags level, string pattern);
 }
