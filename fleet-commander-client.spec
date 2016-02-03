Name:           fleet-commander-client
Version:        0.7.0
Release:        2%{?dist}
Summary:        Fleet Commander Client

License: LGPLv2+
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

Requires: dconf

Requires: systemd
Requires(post): systemd
Requires(preun): systemd
Requires(postun): systemd

%description
Profile data retriever for Fleet Commander client hosts. Fleet Commander is an
application that allows you to manage the desktop configuration of a large
network of users and workstations/laptops.

%prep
%setup -q
%build
%configure
make

%install
%make_install
install -m 755 -d %{buildroot}%{_localstatedir}/cache/fleet-commander

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
%config(noreplace) %{_sysconfdir}/xdg/fleet-commander.conf
%attr(755, -, -) %{_libexecdir}/fleet-commander-client
%{_unitdir}/fleet-commander.service
%attr(755, -, -) %{_localstatedir}/cache/fleet-commander

%changelog
* Wed Feb 03 2016 Alberto Ruiz <aruiz@redhat.com> - 0.7.0-2
- Fix documentation string

* Tue Jan 19 2016 Alberto Ruiz <aruiz@redhat.com> - 0.7.0-1
- Update package for 0.7.0

* Fri Jan 15 2016 Alberto Ruiz <aruiz@redhat.com> - 0.3.0-1
- Initial RPM package
