namespace FleetCommander {
  public class ContentMonitor : Object {
    private File        profiles;
    private FileMonitor monitor;
    private bool        timeout;
    private uint        cookie = 0;
    private uint        interval;

    public signal void content_updated ();

    public ContentMonitor (string path, uint interval = 1000) {
      this.interval = interval;
      profiles = File.new_for_path(path);
      monitor = profiles.monitor_file(FileMonitorFlags.NONE);
      monitor.changed.connect((file, other_file, event) => {
        switch (event) {
          case FileMonitorEvent.CREATED:
          case FileMonitorEvent.CHANGED:
            debug("%s was changed or created", profiles.get_path());
            break;
          case FileMonitorEvent.DELETED:
            debug("%s was deleted", profiles.get_path());
            break;
          default:
            debug("Unhandled FileMonitor event");
            return;
        }
        wait_for_settle (true);
      });
    }

    private void wait_for_settle (bool timeout) {
      this.timeout = timeout;
      if (cookie != 0) {
        debug("A timeout to wait for %s to settle is already installed", profiles.get_path());
        return;
      }

      cookie = Timeout.add (interval, () => {
        if (this.timeout == true) {
          debug("Changes just happened, waiting another second without changes");
          this.timeout = false;
          return true;
        }

        timeout = false;
        cookie  = 0;

        content_updated ();

        return false;
      });
    }
  }
}
