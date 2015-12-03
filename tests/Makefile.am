TESTS_ENVIRONMENT = export FC_PROFILES_PATH=$(abs_srcdir)/data;
TESTS = 02_profile_cache_manager 03_source_manager 04_cache_data 05_dconf_db_writer 06_user_index
check_PROGRAMS = 02_profile_cache_manager

02_profile_cache_manager_SOURCES = \
	test-utils.vapi                                     \
	$(top_srcdir)/service/fc-profile-cache-manager.vala \
	02_profile_cache_manager.vala

02_profile_cache_manager_CFLAGS  = $(JSON_GLIB_CFLAGS) $(GIO_CFLAGS) -g
02_profile_cache_manager_LDADD   = $(JSON_GLIB_LIBS) $(GIO_LIBS)
02_profile_cache_manager_VALAFLAGS = --pkg gio-2.0 --pkg json-glib-1.0


EXTRA_DIST = \
	03_source_manager.vala        \
	04_cache_data.vala            \
	05_dconf_db_writer.vala       \
	06_user_index.vala            \
	tools/dconf                   \
	data/profile-empty.json       \
	data/profile-libreoffice.json \
	data/profile-gsettings.json   \
	data/profile-mixed.json

CLEANFILES = \
	$(TESTS)

03_source_manager_SOURCES = \
	$(top_srcdir)/service/fc-source-manager.vala \
	03_source_manager.vala
03_source_manager: $(03_source_manager_SOURCES)
	valac --pkg gio-2.0 --pkg json-glib-1.0 $^ -o $@

04_cache_data_SOURCES = \
  	test-utils.vapi \
	$(top_srcdir)/service/fc-cache-data.vala \
	04_cache_data.vala
04_cache_data: $(04_cache_data_SOURCES)
	valac --pkg gio-2.0 --pkg json-glib-1.0 $^ -o $@

05_dconf_db_writer_SOURCES = \
  	test-utils.vapi                               \
	cache-data-mock.vala                          \
	$(top_srcdir)/service/fc-dconf-db-writer.vala \
	$(top_srcdir)/service/fc-utils.vala           \
	05_dconf_db_writer.vala
05_dconf_db_writer: $(05_dconf_db_writer_SOURCES)
	valac --pkg gio-2.0 --pkg json-glib-1.0 --pkg posix $^ -o $@

06_user_index_SOURCES = \
	test-utils.vapi                          \
	cache-data-mock.vala                     \
	$(top_srcdir)/service/fc-user-index.vala \
	06_user_index.vala
06_user_index: $(06_user_index_SOURCES)
	valac --pkg gio-2.0 --pkg json-glib-1.0 $^ -o $@
