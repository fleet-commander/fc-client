AC_INIT([fleet-commander],
        [0.1],,
        [fleet-commander])

AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11.2 foreign -Wno-portability no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC

# Use GSettings
GLIB_GSETTINGS

# Dependencies
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.36)  # Just a guess
PKG_CHECK_MODULES(GIO, gio-unix-2.0)

AC_CONFIG_FILES([
  data/Makefile
  service/Makefile
  Makefile
])
AC_OUTPUT
