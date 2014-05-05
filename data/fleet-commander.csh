set dconf_profile=$XDG_RUNTIME_DIR/fleet-commander/dconf_profile

dbus-send --system --dest=org.gnome.FleetCommander --type=method_call /org/gnome/FleetCommander org.gnome.FleetCommander.DConf.WriteProfile string:'$dconf_profile'

if ( $? == 0 ) then
  setenv DCONF_PROFILE $dconf_profile
endif

unset dconf_profile
