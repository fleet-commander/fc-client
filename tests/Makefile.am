TESTS = 02_profile_cache_manager 03_source_manager
check_PROGRAMS = 02_profile_cache_manager

02_profile_cache_manager_SOURCES = \
	test-utils.vapi                                     \
	$(top_srcdir)/service/fc-profile-cache-manager.vala \
	02_profile_cache_manager.vala

02_profile_cache_manager_CFLAGS  = $(JSON_GLIB_CFLAGS) $(GIO_CFLAGS) -g
02_profile_cache_manager_LDADD   = $(JSON_GLIB_LIBS) $(GIO_LIBS)
02_profile_cache_manager_VALAFLAGS = --pkg gio-2.0 --pkg json-glib-1.0


EXTRA_DIST = 03_source_manager.vala
03_source_manager_SOURCES = \
	$(top_srcdir)/service/fc-source-manager.vala \
	03_source_manager.vala
03_source_manager: $(03_source_manager_SOURCES)
	valac --pkg gio-2.0 --pkg json-glib-1.0 $^ -o $@
