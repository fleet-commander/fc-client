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
%{_sysconfdir}/xdg/fleet-commander.conf
#%exclude %{_sysconfdir}/profile.d/fleet-commander.csh
#%exclude %{_sysconfdir}/profile.d/fleet-commander.sh
#%attr(755, -, -) %{_libexecdir}/fcmdr-service
#%attr(755, -, -) %{_libexecdir}/fcmdr-update-goa
%attr(755, -, -) %{_libexecdir}/fleet-commander-client
#%{_datadir}/dbus-1/system-services/org.gnome.FleetCommander.service
%{systemd_dir}/fleet-commander.service

%changelog
