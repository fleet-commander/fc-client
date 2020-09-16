# This package depends on automagic byte compilation
# https://fedoraproject.org/wiki/Changes/No_more_automagic_Python_bytecompilation_phase_2
%global _python_bytecompile_extra 1

Name:           fleet-commander-client
Version:        0.15.0
Release:        3%{?dist}
Summary:        Fleet Commander Client

BuildArch: noarch

License: LGPLv3+ and LGPLv2+ and MIT and BSD
URL: https://raw.githubusercontent.com/fleet-commander/fc-client/master/fleet-commander-client.spec
Source0: https://github.com/fleet-commander/fc-client/releases/download/%{version}/%{name}-%{version}.tar.xz


BuildRequires: dconf

%if 0%{?rhel} && 0%{?rhel} < 8
BuildRequires: python2-devel
BuildRequires: dconf
BuildRequires: pygobject3
BuildRequires: dbus-python
BuildRequires: python-dbusmock
%endif

%if 0%{?fedora} >= 30
BuildRequires: python3-devel
BuildRequires: python3-mock
BuildRequires: python3-gobject
BuildRequires: python3-dbus
BuildRequires: python3-dbusmock
BuildRequires: python3-samba
%endif

%if 0%{?with_check}
BuildRequires: git
BuildRequires: dbus
BuildRequires: python3-mock
BuildRequires: python3-dns
BuildRequires: python3-ldap
BuildRequires: python3-dbusmock
BuildRequires: python3-ipalib
BuildRequires: python3-six
BuildRequires: python3-samba
BuildRequires: NetworkManager-libnm
BuildRequires: json-glib
BuildRequires: NetworkManager
%endif

Requires: NetworkManager
Requires: NetworkManager-libnm
Requires: systemd
Requires: dconf
Requires(preun): systemd

%if 0%{?rhel} && 0%{?rhel} < 8
Requires: python2
Requires: pygobject2
Requires: samba-python
Requires: python-ldap
%endif

%if 0%{?fedora} >= 30
Requires: python3
BuildRequires: python3-gobject
Requires: python3-samba
Requires: python3-dns
Requires: python3-ldap
%endif

%description
Profile data retriever for Fleet Commander client hosts. Fleet Commander is an
application that allows you to manage the desktop configuration of a large
network of users and workstations/laptops.

%prep
%setup -q

%build
%configure --with-systemdsystemunitdir=%{_unitdir}
%configure --with-systemduserunitdir=%{_userunitdir}
%make_build

%install
%make_install

%preun
%systemd_preun fleet-commander-client.service
%systemd_preun fleet-commander-clientad.service
%systemd_user_preun fleet-commander-adretriever.service

%post
%systemd_post fleet-commander-client.service
%systemd_post fleet-commander-clientad.service
%systemd_user_post fleet-commander-adretriever.service

%postun
%systemd_postun_with_restart fleet-commander-client.service
%systemd_postun_with_restart fleet-commander-clientad.service
%systemd_user_postun_with_restart fleet-commander-adretriever.service

%files
%license
%dir %{_datadir}/%{name}
%dir %{_datadir}/%{name}/python
%dir %{_datadir}/%{name}/python/fleetcommanderclient
%attr(644, -, -) %{_datadir}/%{name}/python/fleetcommanderclient/*.py
%dir %{_datadir}/%{name}/python/fleetcommanderclient/configadapters
%attr(644, -, -) %{_datadir}/%{name}/python/fleetcommanderclient/configadapters/*.py
%dir %{_datadir}/%{name}/python/fleetcommanderclient/adapters
%attr(644, -, -) %{_datadir}/%{name}/python/fleetcommanderclient/adapters/*.py
%config(noreplace) %{_sysconfdir}/xdg/%{name}.conf
%config(noreplace) %{_sysconfdir}/dbus-1/system.d/org.freedesktop.FleetCommanderClient.conf
%config(noreplace) %{_sysconfdir}/dbus-1/system.d/org.freedesktop.FleetCommanderClientAD.conf
%{_unitdir}/fleet-commander-client.service
%{_unitdir}/fleet-commander-clientad.service
%{_userunitdir}/fleet-commander-adretriever.service
%{_datadir}/dbus-1/system-services/org.freedesktop.FleetCommanderClient.service
%{_datadir}/dbus-1/system-services/org.freedesktop.FleetCommanderClientAD.service

%if 0%{?rhel} && 0%{?rhel} < 8 
%attr(644, -, -) %{_datadir}/%{name}/python/fleetcommanderclient/*.py[co]
%attr(644, -, -) %{_datadir}/%{name}/python/fleetcommanderclient/configadapters/*.py[co]
%attr(644, -, -) %{_datadir}/%{name}/python/fleetcommanderclient/adapters/*.py[co]
%endif


%changelog
* Tue Jan 28 2020 Fedora Release Engineering <releng@fedoraproject.org> - 0.15.0-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_32_Mass_Rebuild

* Thu Jan 2 2020 Oliver Gutierrez <ogutierrez@redhat.com> - 0.15.0-2
- Removal of python2 dependencies for Fedora

* Mon Dec 23 2019 Oliver Gutierrez <ogutierrez@redhat.com> - 0.15.0-1
- Added Firefox bookmarks deployment

* Mon Oct 21 2019 Miro Hrončok <mhroncok@redhat.com> - 0.14.0-3
- Drop requirement of python2-gobject on Fedora

* Thu Jul 25 2019 Fedora Release Engineering <releng@fedoraproject.org> - 0.14.0-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_31_Mass_Rebuild

* Mon Jul 15 2019 Oliver Gutierrez <ogutierrez@redhat.com> - 0.14.0-1
- Added Active Directory support

* Thu Jan 31 2019 Fedora Release Engineering <releng@fedoraproject.org> - 0.10.2-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_30_Mass_Rebuild

* Fri Jul 13 2018 Fedora Release Engineering <releng@fedoraproject.org> - 0.10.2-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_29_Mass_Rebuild

* Wed Apr 11 2018 Oliver Gutierrez <ogutierrez@redhat.com> - 0.10.2-2
- Fixed building dependencies

* Wed Apr 11 2018 Oliver Gutierrez <ogutierrez@redhat.com> - 0.10.2-1
- Updated package for release 0.10.2

* Thu Mar 01 2018 Iryna Shcherbina <ishcherb@redhat.com> - 0.10.0-4
- Update Python 2 dependency declarations to new packaging standards
  (See https://fedoraproject.org/wiki/FinalizingFedoraSwitchtoPython3)

* Wed Feb 07 2018 Fedora Release Engineering <releng@fedoraproject.org> - 0.10.0-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_28_Mass_Rebuild

* Wed Jul 26 2017 Fedora Release Engineering <releng@fedoraproject.org> - 0.10.0-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_27_Mass_Rebuild

* Mon Jul 10 2017 Oliver Gutierrez <ogutierrez@redhat.com> - 0.10.0-1
- Updated package for release 0.10.0

* Mon Jul 10 2017 Oliver Gutierrez <ogutierrez@redhat.com> - 0.9.1-1
- Code migration to Python
- Updated package for release 0.9.1

* Fri Sep 16 2016 Alberto Ruiz <aruizrui@redhat.com> - 0.8.0-1
- new version

* Wed Jul 20 2016 Alberto Ruiz <aruizrui@redhat.com> - 0.7.1-1
- This release fixes a regression with systemd autostarting the service once
  enabled

* Wed Feb 03 2016 Alberto Ruiz <aruiz@redhat.com> - 0.7.0-2
- Fix documentation string

* Tue Jan 19 2016 Alberto Ruiz <aruiz@redhat.com> - 0.7.0-1
- Update package for 0.7.0

* Fri Jan 15 2016 Alberto Ruiz <aruiz@redhat.com> - 0.3.0-1
- Initial RPM package
