AC_INIT([fleet-commander-client],
        [0.1],,
        [fleet-commander-client])

AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11.2 foreign -Wno-portability no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC

AC_ARG_VAR([DCONF], [path to dconf(1) tool])
AC_PATH_PROG([DCONF], [dconf], [no])

AS_IF([test "x$DCONF" = xno],
      [AC_MSG_ERROR([Could not find 'dconf' in your PATH])])
AC_DEFINE_UNQUOTED(DCONF_PATH, "$DCONF", [Path to dconf(1) tool])

# Use GSettings
GLIB_GSETTINGS

# Dependencies
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.36)  # Just a guess
PKG_CHECK_MODULES(GIO, gio-unix-2.0)
PKG_CHECK_MODULES(JSON_GLIB, json-glib-1.0)
PKG_CHECK_MODULES(LIBSOUP, libsoup-2.4 >= 2.42)  # Just a guess

# Documentation
GTK_DOC_CHECK([1.14])

# systemd
AC_ARG_WITH([systemdsystemunitdir],
            AS_HELP_STRING([--with-systemdsystemunitdir=DIR],
                           [Directory for systemd service files]),
            [],
            [with_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)])
if test "x$with_systemdsystemunitdir" != "xno"; then
  AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])
fi
AM_CONDITIONAL(HAVE_SYSTEMD, [test -n "$systemdsystemunitdir"])

AC_CONFIG_FILES([
  data/Makefile
  data/fleet-commander-client.spec
  docs/Makefile
  service/Makefile
  Makefile
])
AC_OUTPUT
