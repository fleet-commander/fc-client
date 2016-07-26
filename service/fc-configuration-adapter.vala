namespace FleetCommander {
  public interface ConfigurationAdapter : Object {
    public abstract void bootstrap (UserIndex index, Logind.User[] users);
    public abstract void update (UserIndex index, uint32 uid);
  }
}
