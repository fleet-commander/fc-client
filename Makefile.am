NULL =

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = data docs service-ng

EXTRA_DIST = m4

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-gtk-doc \
	--with-systemdsystemunitdir=$$dc_install_base/$(systemdsystemunitdir) \
	$(NULL)

MAINTAINERCLEANFILES = \
	$(srcdir)/aclocal.m4 \
	$(srcdir)/config.h.in \
	$(srcdir)/depcomp \
	$(srcdir)/install-sh \
	$(srcdir)/missing \
	$(srcdir)/docs/gtk-doc.make \
	$(srcdir)/m4/gtk-doc.m4 \
	$(NULL)

-include $(top_srcdir)/git.mk
