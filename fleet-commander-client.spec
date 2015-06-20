Name:           fleet-commander-client
Version:        0.2.0
Release:        1%{?dist}
Summary:        Fleet Commander Client

License: LGPL-2.1+
URL: https://github.com/fleet-commander/fc-client
Source0: https://github.com/fleet-commander/fc-client/releases/download/%{version}/%{name}-%{version}.tar.xz

BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(gio-unix-2.0)
BuildRequires: pkgconfig(json-glib-1.0)
BuildRequires: pkgconfig(libsoup-2.4)
BuildRequires: pkgconfig(goa-1.0)
BuildRequires: gtk-doc
BuildRequires: dconf
BuildRequires: systemd

Requires: glib2
Requires: json-glib
Requires: libsoup
Requires: gnome-online-accounts
Requires: dconf

Requires: systemd
Requires(post): systemd
Requires(preun): systemd
Requires(postun): systemd

%define systemd_dir %{_prefix}/lib/systemd/system

%description
Profile data retreiver for Fleet Commander client hosts

%prep
%setup -q
%build
%configure --with-systemdsystemunitdir=%{systemd_dir}
make

%install
%make_install

%clean
rm -rf %{buildroot}

%post
%systemd_post fleet-commander.service

%preun
%systemd_preun fleet-commander.service

%postun
%systemd_postun_with_restart fleet-commander.service 

%files
%defattr(644, root, root)
/etc/dbus-1/system.d/org.gnome.FleetCommander.conf
%attr(755, -, -) /etc/profile.d/fleet-commander.csh
%attr(755, -, -) /etc/profile.d/fleet-commander.sh
/usr/lib/systemd/system/fleet-commander.service
%attr(755, -, -) /usr/libexec/fcmdr-service
%attr(755, -, -) /usr/libexec/fcmdr-update-goa
/usr/share/dbus-1/system-services/org.gnome.FleetCommander.service
/etc/xdg/fleet-commander.conf

%package doc
Summary: Fleet Commander client daemon documentation

%description doc
Fleet Commander client daemon documentation

%files doc
%defattr(644, root, root)
%{_datarootdir}/gtk-doc/html/fleet-commander/FCmdrProfile.html
%{_datarootdir}/gtk-doc/html/fleet-commander/FCmdrProfileSource.html
%{_datarootdir}/gtk-doc/html/fleet-commander/FCmdrService.html
%{_datarootdir}/gtk-doc/html/fleet-commander/FCmdrServiceBackend.html
%{_datarootdir}/gtk-doc/html/fleet-commander/FCmdrUserResolver.html
%{_datarootdir}/gtk-doc/html/fleet-commander/api-index-full.html
%{_datarootdir}/gtk-doc/html/fleet-commander/ch01.html
%{_datarootdir}/gtk-doc/html/fleet-commander/deprecated-api-index.html
%{_datarootdir}/gtk-doc/html/fleet-commander/fleet-commander-Miscellaneous-Utilities.html
%{_datarootdir}/gtk-doc/html/fleet-commander/fleet-commander.devhelp2
%{_datarootdir}/gtk-doc/html/fleet-commander/home.png
%{_datarootdir}/gtk-doc/html/fleet-commander/index.html
%{_datarootdir}/gtk-doc/html/fleet-commander/index.sgml
%{_datarootdir}/gtk-doc/html/fleet-commander/left-insensitive.png
%{_datarootdir}/gtk-doc/html/fleet-commander/left.png
%{_datarootdir}/gtk-doc/html/fleet-commander/object-tree.html
%{_datarootdir}/gtk-doc/html/fleet-commander/right-insensitive.png
%{_datarootdir}/gtk-doc/html/fleet-commander/right.png
%{_datarootdir}/gtk-doc/html/fleet-commander/style.css
%{_datarootdir}/gtk-doc/html/fleet-commander/up-insensitive.png
%{_datarootdir}/gtk-doc/html/fleet-commander/up.png

%changelog
