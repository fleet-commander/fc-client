AC_INIT(fleet-commander-client, 0.9.1, aruiz@redhat.com)
AC_COPYRIGHT([Copyright 2014,2015,2016,2017 Red Hat, Inc.])

AC_PREREQ(2.64)
AM_INIT_AUTOMAKE([no-dist-gzip dist-xz])
AM_MAINTAINER_MODE
AC_CONFIG_MACRO_DIR([m4])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PATH_PROG([RUNUSER], [runuser])
AC_PATH_PROG([MKDIR], [mkdir])

if test x$RUNUSER = x ; then
  AC_MSG_ERROR([Could not find runuser])
fi

PKG_PROG_PKG_CONFIG
AC_ARG_WITH([systemdsystemunitdir],
     [AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files])],,
     [with_systemdsystemunitdir=auto])
AS_IF([test "x$with_systemdsystemunitdir" = "xyes" -o "x$with_systemdsystemunitdir" = "xauto"], [
     def_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)

     AS_IF([test "x$def_systemdsystemunitdir" = "x"],
   [AS_IF([test "x$with_systemdsystemunitdir" = "xyes"],
    [AC_MSG_ERROR([systemd support requested but pkg-config unable to query systemd package])])
    with_systemdsystemunitdir=no],
   [with_systemdsystemunitdir="$def_systemdsystemunitdir"])])
AS_IF([test "x$with_systemdsystemunitdir" != "xno"],
      [AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])])
AM_CONDITIONAL([HAVE_SYSTEMD], [test "x$with_systemdsystemunitdir" != "xno"])

if test "x$with_systemdsystemunitdir" = "xno"; then
  AC_MSG_ERROR([systemd support is mandatory])
fi


################
# Dependencies #
################

AM_PATH_PYTHON([2],, [:])
AC_PYTHON_MODULE([dbus], [mandatory])
AC_PYTHON_MODULE([gobject], [mandatory])
AC_PYTHON_MODULE([gi], [mandatory])
AC_PYTHON_MODULE([dbusmock])

# libexecdir expansion for .desktop file
# TODO: Make xdgconfigdir parametric
privlibexecdir='${libexecdir}'
xdgconfigdir='${sysconfdir}'/xdg
clientstatedir='${localstatedir}'/lib/fleet-commander-client
fcclientdir='${datarootdir}'/fleet-commander-client
fcpythondir='${datarootdir}'/fleet-commander-client/python

AC_SUBST(privlibexecdir)
AC_SUBST(xdgconfigdir)
AC_SUBST(clientstatedir)
AC_SUBST(fcclientdir)
AC_SUBST(fcpythondir)

AS_AC_EXPAND(XDGCONFIGDIR, "$xdgconfigdir")
AS_AC_EXPAND(PRIVLIBEXECDIR, "$privlibexecdir")
AS_AC_EXPAND(CLIENTSTATEDIR, "$clientstatedir")
AS_AC_EXPAND(FCCLIENTDIR, "$fcclientdir")
AS_AC_EXPAND(FCPYTHONDIR, "$fcpythondir")

AC_SUBST(SYSTEMUNITDIR)

AC_OUTPUT([
Makefile
data/Makefile
tests/Makefile
src/Makefile
data/fleet-commander-client.service
data/org.freedesktop.FleetCommanderClient.service
])
