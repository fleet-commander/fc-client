include gtk-doc.make

NULL =

DOC_MODULE = fleet-commander

DOC_MAIN_SGML_FILE = fleet-commander-docs.xml

DOC_SOURCE_DIR = $(top_srcdir)/service
HFILE_GLOB = $(top_srcdir)/service/*.h
CFILE_GLOB = $(top_srcdir)/service/*.c

MKDB_OPTIONS = --output-format=xml --sgml-mode

# Skip generated code and extensions.
# Just document the core API for now.
IGNORE_HFILES = \
	fcmdr-extensions.h \
	fcmdr-generated.h \
	fcmdr-gsettings-backend.h \
	fcmdr-http-profile-source.h \
	$(NULL)

GTKDOC_CFLAGS = \
	$(JSON_GLIB_CFLAGS) \
	$(LIBSOUP_CFLAGS) \
	$(GIO_CFLAGS) \
	$(NULL)

GTKDOC_LIBS = \
	$(top_builddir)/service/fcmdr_service-fcmdr-*.o \
	$(JSON_GLIB_LIBS) \
	$(LIBSOUP_LIBS) \
	$(GIO_LIBS) \
	$(NULL)

-include $(top_srcdir)/git.mk
