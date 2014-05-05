dconf_profile=$XDG_RUNTIME_DIR/fleet-commander/dconf_profile

dbus-send --system --dest=org.gnome.FleetCommander --type=method_call /org/gnome/FleetCommander org.gnome.FleetCommander.DConf.WriteProfile string:'$dconf_profile'

if [ $? -eq 0 ]
  then DCONF_PROFILE=$dconf_profile
  export DCONF_PROFILE
fi
