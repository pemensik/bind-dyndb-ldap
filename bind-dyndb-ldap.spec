Name:           bind-dyndb-ldap
Version:        0.1.0
Release:        0.3.a1%{?dist}
Summary:        LDAP back-end plug-in for BIND

Group:          System Environment/Libraries
License:        GPLv2
URL:            http://mnagy.github.com/bind-dyndb-ldap
Source0:        http://cloud.github.com/downloads/mnagy/%{name}/%{name}-%{version}a1.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  bind-devel >= 9.6.1-0.3.b1
BuildRequires:  openldap-devel
Requires:       bind >= 9.6.1-0.3.b1

%description
This package provides an LDAP back-end plug-in for BIND. It features
support for dynamic updates and internal caching, to lift the load
off of your LDAP server.


%prep
%setup -q -n %{name}-%{version}a1


%build
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
%doc README COPYING doc/{example.ldif,schema}
%{_libdir}/bind/ldap.so


%changelog
* Fri Jul 24 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.1.0-0.3.a1
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Fri Jun 19 2009 Caolán McNamara <caolanm@redhat.com> - 0.1.0-0.2.a1
- rebuild for dependencies

* Sun May 03 2009 Martin Nagy <mnagy@redhat.com> - 0.1.0-0.1.a1
- initial packaging
