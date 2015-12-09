NULL =

serviceconfdir = $(sysconfdir)/xdg/
serviceconf_DATA = fleet-commander.conf

if HAVE_SYSTEMD
systemdservicedir = $(systemdsystemunitdir)
systemdservice_DATA = fleet-commander.service

fleet-commander.service: Makefile
	$(AM_V_GEN) (echo '[Unit]'; \
	             echo 'Description=Fleet Commander client configuration management service'; \
	             echo 'After=network.target remote-fs.target nss-lookup.target'; \
	             echo '';                                                        \
	             echo '[Service]';                                               \
	             echo 'ExecStart=${libexecdir}/fleet-commander-client';          \
	             echo 'StandardOutput=syslog';                                   \
	             echo 'StandardError=inherit';                                   \
	             echo '';                                                        \
		     echo '[Install]';                                               \
		     echo 'Alias=fleet-commander.service') > $@.tmp &&               \
	            mv $@.tmp $@
endif

CLEANFILES = $(systemdservice_DATA)

EXTRA_DIST = $(serviceconf_DATA)

-include $(top_srcdir)/git.mk
