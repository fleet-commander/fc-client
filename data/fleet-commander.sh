if [ -z "$XDG_RUNTIME_DIR" ]; then
  # Fall back in the same manner as GLib.
  if [ -n "$XDG_CACHE_HOME" ]; then
    XDG_RUNTIME_DIR="$XDG_CACHE_HOME"
  else
    XDG_RUNTIME_DIR="$HOME/.cache"
  fi
fi

dconf_profile=$XDG_RUNTIME_DIR/fleet-commander/dconf_profile

mkdir -p `dirname $dconf_profile`

# Use --print-reply to wait for a reply message.
# This ensures the service can lookup the user ID.
dbus-send --system --print-reply --dest=org.gnome.FleetCommander --type=method_call /org/gnome/FleetCommander org.gnome.FleetCommander.DConf.WriteProfile "string:$dconf_profile" > /dev/null

if [ $? -eq 0 ]; then
  DCONF_PROFILE=$dconf_profile
  export DCONF_PROFILE
fi
