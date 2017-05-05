Name:           fleet-commander-client
Version:        0.9.0
Release:        1%{?dist}
Summary:        Fleet Commander Client

BuildArch: noarch

License: LGPLv3+ and LGPLv2+ and MIT and BSD
URL: https://raw.githubusercontent.com/fleet-commander/fc-client/master/fleet-commander-client.spec
Source0: https://github.com/fleet-commander/fc-client/releases/download/%{version}/%{name}-%{version}.tar.xz

BuildRequires: python2-devel
BuildRequires: dbus-python
BuildRequires: pygobject2
BuildRequires: python-dbusmock
BuildRequires: dconf
%if 0%{?rhel} < 8
BuildRequires: pygobject3
%endif
%if 0%{?fedora} >= 21
BuildRequires: python-gobject
%endif

Requires: NetworkManager
Requires: NetworkManager-libnm
Requires: systemd
Requires: dconf
Requires: python2
Requires: dbus-python
Requires: pygobject2
Requires(preun): systemd
%if 0%{?rhel} < 8
Requires: pygobject3
%endif
%if 0%{?fedora} >= 21
Requires: python-gobject
%endif

%description
Profile data retriever for Fleet Commander client hosts. Fleet Commander is an
application that allows you to manage the desktop configuration of a large
network of users and workstations/laptops.

%prep
%setup -q

%build
%configure --with-systemdsystemunitdir=%{_unitdir}
%make_build

%install
%make_install

%preun
%systemd_preun fleet-commander-client.service

%post
%systemd_post fleet-commander-client.service

%postun
%systemd_postun_with_restart fleet-commander-client.service

%files
%license
%dir %{_datadir}/%{name}
%dir %{_datadir}/%{name}/python
%dir %{_datadir}/%{name}/python/fleetcommanderclient
%attr(644, -, -) %{_datadir}/%{name}/python/fleetcommanderclient/*.py
%attr(644, -, -) %{_datadir}/%{name}/python/fleetcommanderclient/*.py[co]
%dir %{_datadir}/%{name}/python/fleetcommanderclient/configadapters
%attr(644, -, -) %{_datadir}/%{name}/python/fleetcommanderclient/configadapters/*.py
%attr(644, -, -) %{_datadir}/%{name}/python/fleetcommanderclient/configadapters/*.py[co]
%config(noreplace) %{_sysconfdir}/xdg/%{name}.conf
%config(noreplace) %{_sysconfdir}/dbus-1/system.d/org.freedesktop.FleetCommanderClient.conf
%{_unitdir}/fleet-commander-client.service
%{_datadir}/dbus-1/system-services/org.freedesktop.FleetCommanderClient.service

%changelog
* Fri Sep 16 2016 Alberto Ruiz <aruizrui@redhat.com> - 0.8.0-1
- new version

* Wed Feb 03 2016 Alberto Ruiz <aruiz@redhat.com> - 0.7.0-2
- Fix documentation string

* Tue Jan 19 2016 Alberto Ruiz <aruiz@redhat.com> - 0.7.0-1
- Update package for 0.7.0

* Fri Jan 15 2016 Alberto Ruiz <aruiz@redhat.com> - 0.3.0-1
- Initial RPM package
