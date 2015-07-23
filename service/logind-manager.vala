namespace Logind {
  public struct User {
    uint32 uid;
    string name;
    ObjectPath path;
  }

  [DBus (name = "org.freedesktop.login1.Manager")]
  public interface Manager : GLib.Object {
    public abstract User[] list_users () throws IOError;
    public abstract signal void user_new     (uint32 uid, ObjectPath path);
    public abstract signal void user_removed (uint32 uid, ObjectPath path);
  }
}

