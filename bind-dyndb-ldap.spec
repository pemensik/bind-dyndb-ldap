#%define PATCHVER P4
%define PREVER b1
#%define VERSION %{version}
#%define VERSION %{version}-%{PATCHVER}
%define VERSION %{version}%{PREVER}

Name:           bind-dyndb-ldap
Version:        1.1.0
Release:        0.9.%{PREVER}%{?dist}
Summary:        LDAP back-end plug-in for BIND

Group:          System Environment/Libraries
License:        GPLv2+
URL:            https://fedorahosted.org/bind-dyndb-ldap
Source0:        https://fedorahosted.org/released/%{name}/%{name}-%{VERSION}.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  bind-devel >= 32:9.6.1-0.3.b1
BuildRequires:  krb5-devel
BuildRequires:  openldap-devel

Requires:       bind >= 32:9.6.1-0.3.b1

%description
This package provides an LDAP back-end plug-in for BIND. It features
support for dynamic updates and internal caching, to lift the load
off of your LDAP server.


%prep
%setup -q -n %{name}-%{VERSION}

%build
export CFLAGS="`isc-config.sh --cflags dns` $RPM_OPT_FLAGS"
%configure
make %{?_smp_mflags}


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}

# Remove unwanted files
rm %{buildroot}%{_libdir}/bind/ldap.la
rm -r %{buildroot}%{_datadir}/doc/%{name}


%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root,-)
%doc NEWS README COPYING doc/{example.ldif,schema}
%{_libdir}/bind/ldap.so


%changelog
* Tue Mar 06 2012 Adam Tkac <atkac redhat com> - 1.1.0-0.9.b1
- update to 1.1.0b1

* Mon Feb 13 2012 Adam Tkac <atkac redhat com> - 1.1.0-0.8.a2
- update to 1.1.0a2

* Thu Feb 02 2012 Adam Tkac <atkac redhat com> - 1.1.0-0.7.a1
- rebuild against new bind

* Wed Jan 18 2012 Adam Tkac <atkac redhat com> - 1.1.0-0.6.a1
- update to 1.1.0a1

* Thu Jan 12 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.0.0-0.5.rc1
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Mon Nov 14 2011 Adam Tkac <atkac redhat com> - 1.0.0-0.4.rc1
- update to 1.0.0rc1

* Mon Nov 14 2011 Adam Tkac <atkac redhat com> - 1.0.0-0.3.b1
- rebuild against new bind

* Fri Sep 09 2011 Adam Tkac <atkac redhat com> - 1.0.0-0.2.b1
- rebuild against new bind

* Wed Aug 31 2011 Adam Tkac <atkac redhat com> - 1.0.0-0.1.b1
- update to 1.0.0b1 (psearch + bugfixes)
- bind-dyndb-ldap-rh727856.patch merged

* Wed Aug 03 2011 Adam Tkac <atkac redhat com> - 0.2.0-4
- fix race condition in semaphore_wait (#727856)

* Mon Feb 21 2011 Adam Tkac <atkac redhat com> - 0.2.0-3
- rebuild against new bind

* Mon Feb 07 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.2.0-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Wed Jan 12 2011 Adam Tkac <atkac redhat com> - 0.2.0-1
- update to 0.2.0
- patches merged
  - 0001-Bugfix-Improve-LDAP-schema-to-be-loadable-by-OpenLDA.patch
  - 0004-Bugfix-Fix-loading-of-child-zones-from-LDAP.patch

* Wed Dec 15 2010 Adam Tkac <atkac redhat com> - 0.1.0-0.17.b
- fix LDAP schema (#622604)
- load child zones from LDAP correctly (#622617)

* Fri Oct 22 2010 Adam Tkac <atkac redhat com> - 0.1.0-0.16.b
- build with correct RPM_OPT_FLAGS (#645529)

* Wed Oct 20 2010 Adam Tkac <atkac redhat com> - 0.1.0-0.15.b
- use "isc-config.sh" utility to get correct BIND9 CFLAGS

* Thu Sep 30 2010 Adam Tkac <atkac redhat com> - 0.1.0-0.14.b
- rebuild against new bind

* Fri Aug 27 2010 Adam Tkac <atkac redhat com> - 0.1.0-0.13.b
- rebuild against new bind

* Tue Aug 17 2010 Adam Tkac <atkac redhat com> - 0.1.0-0.12.b
- rebuild against new bind

* Tue Aug 03 2010 Adam Tkac <atkac redhat com> - 0.1.0-0.11.b
- rebuild against new bind

* Mon May 31 2010 Adam Tkac <atkac redhat com> - 0.1.0-0.10.b
- rebuild against new bind

* Wed Mar 24 2010 Martin Nagy <mnagy@redhat.com> - 0.1.0-0.9.b
- update to the latest upstream release

* Thu Jan 28 2010 Adam Tkac <atkac redhat com> - 0.1.0-0.8.a1.20091210git
- rebuild against new bind

* Tue Dec 15 2009 Adam Tkac <atkac redhat com> - 0.1.0-0.7.a1.20091210git
- rebuild against new bind

* Thu Dec 10 2009 Martin Nagy <mnagy@redhat.com> - 0.1.0-0.6.a1.20091210git
- update to the latest git snapshot
- change upstream URL, project moved to fedorahosted
- change license to GPL version 2 or later
- add epoch to versioned requires
- add krb5-devel to the list of build requires

* Tue Dec 01 2009 Adam Tkac <atkac redhat com> - 0.1.0-0.5.a1
- rebuild against new bind

* Thu Nov 26 2009 Adam Tkac <atkac redhat com> - 0.1.0-0.4.a1
- rebuild against new bind

* Fri Jul 24 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.1.0-0.3.a1
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Fri Jun 19 2009 Caolán McNamara <caolanm@redhat.com> - 0.1.0-0.2.a1
- rebuild for dependencies

* Sun May 03 2009 Martin Nagy <mnagy@redhat.com> - 0.1.0-0.1.a1
- initial packaging
