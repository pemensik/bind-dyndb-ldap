%define VERSION %{version}

Name:           bind-dyndb-ldap
Version:        8.0
Release:        4%{?dist}
Summary:        LDAP back-end plug-in for BIND

Group:          System Environment/Libraries
License:        GPLv2+
URL:            https://fedorahosted.org/bind-dyndb-ldap
Source0:        https://fedorahosted.org/released/%{name}/%{name}-%{VERSION}.tar.bz2
Source1:        https://fedorahosted.org/released/%{name}/%{name}-%{VERSION}.tar.bz2.asc
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  bind-devel >= 32:9.9.0-1, bind-lite-devel >= 32:9.9.0-1
BuildRequires:  krb5-devel
BuildRequires:  openldap-devel
BuildRequires:  libuuid-devel
BuildRequires:  automake, autoconf, libtool

Requires:       bind-pkcs11 >= 32:9.9.6-2, bind-pkcs11-utils >= 32:9.9.6-2

%description
This package provides an LDAP back-end plug-in for BIND. It features
support for dynamic updates and internal caching, to lift the load
off of your LDAP server.


%prep
%setup -q -n %{name}-%{VERSION}

%build
export CFLAGS="`isc-config.sh --cflags dns` $RPM_OPT_FLAGS"
autoreconf -fiv
%configure
make %{?_smp_mflags}


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
mkdir -m 770 -p %{buildroot}/%{_localstatedir}/named/dyndb-ldap

# Remove unwanted files
rm %{buildroot}%{_libdir}/bind/ldap.la
rm -r %{buildroot}%{_datadir}/doc/%{name}


# SELinux boolean named_write_master_zones has to be enabled
# otherwise the plugin will not be able to write to /var/named.
# This scriptlet enables the boolean after installation or upgrade.
# SELinux is sensitive area so I want to inform user about the change.
%post
if [ -x "/usr/sbin/setsebool" ] ; then
        echo "Enabling SELinux boolean named_write_master_zones"
        /usr/sbin/setsebool -P named_write_master_zones=1 || :
fi


# This scriptlet disables the boolean after uninstallation.
%postun
if [ "0$1" -eq "0" ] && [ -x "/usr/sbin/setsebool" ] ; then
        echo "Disabling SELinux boolean named_write_master_zones"
        /usr/sbin/setsebool -P named_write_master_zones=0 || :
fi


%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root,-)
%doc NEWS README COPYING doc/{example,schema}.ldif
%dir %attr(770, root, named) %{_localstatedir}/named/dyndb-ldap
%{_libdir}/bind/ldap.so


%changelog
* Thu Dec 17 2015 Petr Spacek <pspacek@redhat.com> - 8.0-4
- Rebuild against bind 9.10.3-P2

* Fri Sep 04 2015 Tomas Hozza <thozza@redhat.com> - 8.0-3
- Rebuild against bind 9.10.3rc1

* Wed Jun 24 2015 Tomas Hozza <thozza@redhat.com> - 8.0-2
- rebuild against bind-9.10.2-P1

* Tue Jun 23 2015 Petr Spacek <pspacek@redhat.com> - 8.0-1
- update to 8.0

* Wed Jun 17 2015 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 7.0-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_23_Mass_Rebuild

* Fri Mar 13 2015 Tomas Hozza <thozza@redhat.com> - 7.0-4
- rebuild against bind-9.10.2

* Wed Feb 25 2015 Tomas Hozza <thozza@redhat.com> - 7.0-3
- Rebuild against bind-9.10.2rc2

* Mon Feb 02 2015 Tomas Hozza <thozza@redhat.com> - 7.0-2
- rebuild against bind-9.10.2rc1

* Mon Jan 12 2015 Petr Spacek <pspacek@redhat.com> - 7.0-1
- update to 7.0 to add support for BIND 9.10

* Tue Dec 02 2014 Petr Spacek <pspacek@redhat.com> - 6.1-1
- update to 6.1
- drop patches which were merged upstream

* Tue Oct 21 2014 Petr Viktorin <pviktori@redhat.com> - 6.0-5
- use lower version of bind-pkcs11-utils for f20 and el7

* Mon Oct 20 2014 Petr Spacek <pspacek@redhat.com> - 6.0-4
- add dependency on bind-pkcs11-utils >= 32:9.9.6-2
  to help with freeipa-server upgrade

* Mon Oct 20 2014 Petr Spacek <pspacek@redhat.com> - 6.0-3
- replace dependency on bind with dependency on bind-pkcs11 >= 32:9.9.6-2
  to help with freeipa-server upgrade

* Fri Oct 03 2014 Tomas Hozza <thozza@redhat.com> - 6.0-2
- rebuild against bind-9.9.6

* Tue Sep 23 2014 Petr Spacek <pspacek redhat com> - 6.0-1
- update to 6.0

* Fri Sep 12 2014 Petr Spacek <pspacek redhat com> - 5.3-1
- update to 5.3

* Mon Sep 08 2014 Petr Spacek <pspacek redhat com> 5.2-1
- update to 5.2

* Fri Aug 15 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 5.1-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_22_Mass_Rebuild

* Thu Jul 24 2014 Petr Spacek <pspacek redhat com> 5.1-1
- update to 5.1
- fixes bug 1122393

* Tue Jun 24 2014 Petr Spacek <pspacek redhat com> 5.0-1
- update to 5.0

* Sat Jun 07 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 4.3-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_Mass_Rebuild

* Wed Apr 09 2014 Petr Spacek <pspacek redhat com> 4.3-1
- update to 4.3

* Mon Feb 24 2014 Petr Spacek <pspacek redhat com> 4.1-2
- remove deprecated define _BSD_SOURCE

* Mon Feb 24 2014 Petr Spacek <pspacek redhat com> 4.1-1
- update to 4.1

* Thu Jul 18 2013 Petr Spacek <pspacek redhat com> 3.5-1
- update to 3.5

* Mon Jul 15 2013 Tomas Hozza <thozza@redhat.com> 3.4-2
- rebuild against new bind

* Tue Jun 25 2013 Petr Spacek <pspacek redhat com> 3.4-1
- update to 3.4

* Tue Jun 04 2013 Petr Spacek <pspacek redhat com> 3.3-1
- update to 3.3
- patch bind-dyndb-ldap-tbabej-0001-Build-fixes-for-Fedora-19.patch merged

* Tue May 14 2013 Petr Spacek <pspacek redhat com> 3.2-1
- update to 3.2

* Tue Apr 16 2013 Adam Tkac <atkac redhat com> 3.1-2
- rebuild against new bind
- build with --disable-werror

* Fri Apr 12 2013 Petr Spacek <pspacek redhat com> 3.1-1
- update to 3.1

* Tue Apr 02 2013 Petr Spacek <pspacek redhat com> 3.0-1
- update to 3.0

* Tue Mar 26 2013 Petr Spacek <pspacek redhat com> 2.6-1
- update to 2.6

* Mon Feb 04 2013 Petr Spacek <pspacek redhat com> 2.5-1
- update to 2.5

* Tue Jan 15 2013 Petr Spacek <pspacek redhat com> 2.4-1
- update to 2.4

* Thu Nov  8 2012 Petr Spacek <pspacek redhat com> 2.3-2
- rebuild with proper changelog

* Thu Nov  8 2012 Petr Spacek <pspacek redhat com> 2.3-1
- update to 2.3

* Mon Oct 29 2012 Adam Tkac <atkac redhat com> 2.1-1
- update to 2.1

* Thu Oct 11 2012 Adam Tkac <atkac redhat com> 2.0-0.3.20121009git6a86b1
- rebuild against new bind-libs

* Tue Oct  9 2012 Petr Spacek <pspacek redhat com> 2.0-0.2.20121009git6a86b1
- update to the latest master

* Fri Sep 21 2012 Adam Tkac <atkac redhat com> 2.0-0.1.20120921git7710d89
- update to the latest master
- bind-dyndb-ldap110-master.patch was merged

* Thu Aug 16 2012 Adam Tkac <atkac redhat com> 1.1.0-0.16.rc1
- update to the latest git

* Thu Aug 02 2012 Adam Tkac <atkac redhat com> 1.1.0-0.15.rc1
- update to the latest git
  - fix for CVE-2012-3429 has been merged

* Thu Aug 02 2012 Adam Tkac <atkac redhat com> 1.1.0-0.14.rc1
- fix CVE-2012-3429

* Wed Jul 18 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.1.0-0.13.rc1
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Thu Jun 07 2012 Adam Tkac <atkac redhat com> - 1.1.0-0.12.rc1
- update to the latest master (#827401)

* Thu Apr 26 2012 Adam Tkac <atkac redhat com> - 1.1.0-0.11.rc1
- update to 1.1.0rc1 (CVE-2012-2134)

* Tue Mar 27 2012 Adam Tkac <atkac redhat com> - 1.1.0-0.10.b2
- update to 1.1.0b2

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
