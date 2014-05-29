set dconf_profile=$XDG_RUNTIME_DIR/fleet-commander/dconf_profile

mkdir -p `dirname $dconf_profile`

# Use --print-reply to wait for a reply message.
# This ensures the service can lookup the user ID.
dbus-send --system --print-reply --dest=org.gnome.FleetCommander --type=method_call /org/gnome/FleetCommander org.gnome.FleetCommander.DConf.WriteProfile "string:$dconf_profile" > /dev/null

if ( $? == 0 ) then
  setenv DCONF_PROFILE $dconf_profile
endif

unset dconf_profile
