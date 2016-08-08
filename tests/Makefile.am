TESTS_ENVIRONMENT = export FC_PROFILES_PATH=$(abs_srcdir)/data;
TESTS = 01_config_reader 02_profile_cache_manager 03_source_manager 04_cache_data 05_dconf_db_writer 06_user_index 07_adapter_dconf_profiles 08_adapter_goa 09_adapter_nm
VALAC_FLAGS=--pkg gio-2.0 --pkg json-glib-1.0 --pkg posix -g #-X -g
EXTRA_DIST = \
	01_config_reader.vala         \
	02_profile_cache_manager.vala \
	03_source_manager.vala        \
	04_cache_data.vala            \
	05_dconf_db_writer.vala       \
	06_user_index.vala            \
	07_adapter_dconf_profiles.vala \
	08_adapter_goa.vala           \
	09_adapter_nm.vala            \
	test-utils.vapi               \
	cache-data-mock.vala          \
	posix-mock.vala               \
	data/profile-empty.json       \
	data/profile-libreoffice.json \
	data/profile-gsettings.json   \
	data/profile-mixed.json       \
	data/profile-goa.json         \
	data/profile-moar.json        \
	data/profile-simple.json      \
	data/profile-dumb.json        \
	data/applies-simple.json      \
	data/applies-duplicates.json  \
	data/applies-goa.json         \
	data/applies-empty.json       \
	data/applies-libreoffice.json \
	data/simple-test.conf         \
	data/interval-zero.conf       \
	data/missing-group.conf

CLEANFILES = \
	$(TESTS)

01_config_reader_SOURCES = \
  	test-utils.vapi                             \
  	$(top_srcdir)/service/fc-config-reader.vala \
	01_config_reader.vala
01_config_reader: $(01_config_reader_SOURCES)
	valac $(VALAC_FLAGS) $^ -o $@

02_profile_cache_manager_SOURCES = \
	test-utils.vapi                                     \
	$(top_srcdir)/service/fc-profile-cache-manager.vala \
	02_profile_cache_manager.vala
02_profile_cache_manager: $(02_profile_cache_manager_SOURCES)
	valac $(VALAC_FLAGS) $^ -o $@

03_source_manager_SOURCES = \
	test-utils.vapi	                              \
	$(top_srcdir)/service/fc-source-manager.vala \
	03_source_manager.vala
03_source_manager: $(03_source_manager_SOURCES)
	valac $(VALAC_FLAGS) $^ -o $@

04_cache_data_SOURCES = \
  	test-utils.vapi \
	$(top_srcdir)/service/fc-cache-data.vala \
	04_cache_data.vala
04_cache_data: $(04_cache_data_SOURCES)
	valac $(VALAC_FLAGS) $^ -o $@

05_dconf_db_writer_SOURCES = \
  	test-utils.vapi                               \
	cache-data-mock.vala                          \
	$(top_srcdir)/service/fc-dconf-db-writer.vala \
	$(top_srcdir)/service/fc-utils.vala           \
	05_dconf_db_writer.vala
05_dconf_db_writer: $(05_dconf_db_writer_SOURCES)
	valac $(VALAC_FLAGS) $^ -o $@

06_user_index_SOURCES = \
	test-utils.vapi                          \
	cache-data-mock.vala                     \
	$(top_srcdir)/service/fc-user-index.vala \
	06_user_index.vala
06_user_index: $(06_user_index_SOURCES)
	valac $(VALAC_FLAGS) $^ -o $@

07_adapter_dconf_profiles_SOURCES = \
	test-utils.vapi             \
	cache-data-mock.vala        \
	posix-mock.vala             \
	$(top_srcdir)/service/fc-utils.vala                                \
	$(top_srcdir)/service/fc-user-index.vala                           \
	$(top_srcdir)/service/fc-configuration-adapter.vala                \
	$(top_srcdir)/service/fc-configuration-adapter-dconf-profiles.vala \
	$(top_srcdir)/service/logind-manager.vala                          \
	07_adapter_dconf_profiles.vala

07_adapter_dconf_profiles: $(07_adapter_dconf_profiles_SOURCES)
	valac $(VALAC_FLAGS) $^ -o $@

08_adapter_goa_SOURCES = \
	test-utils.vapi             \
	cache-data-mock.vala        \
	posix-mock.vala             \
	$(top_srcdir)/service/fc-utils.vala                     \
	$(top_srcdir)/service/fc-user-index.vala                \
	$(top_srcdir)/service/fc-configuration-adapter.vala     \
	$(top_srcdir)/service/fc-configuration-adapter-goa.vala \
	$(top_srcdir)/service/logind-manager.vala               \
	08_adapter_goa.vala

08_adapter_goa: $(08_adapter_goa_SOURCES)
	valac $(VALAC_FLAGS) $^ -o $@

09_adapter_nm_SOURCES = \
	test-utils.vapi             \
	cache-data-mock.vala        \
	posix-mock.vala             \
	$(top_srcdir)/service/fc-utils.vala                     \
	$(top_srcdir)/service/fc-user-index.vala                \
	$(top_srcdir)/service/fc-configuration-adapter.vala     \
	$(top_srcdir)/service/fc-configuration-adapter-nm.vala  \
	$(top_srcdir)/service/logind-manager.vala               \
	$(top_srcdir)/service/network-manager.vala              \
	09_adapter_nm.vala

09_adapter_nm: $(09_adapter_nm_SOURCES)
	valac $(VALAC_FLAGS) $^ -o $@
