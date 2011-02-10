Name:		php-libvirt
Version:	0.4
Release:	1%{?dist}%{?extra_release}
Summary:	PHP language binding for Libvirt
Group:		Development/Libraries
License:	PHP
URL:		http://libvirt.org/
Source0:	http://libvirt.org/sources/libvirt-php-%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:	php-devel
BuildRequires:	libvirt-devel
BuildRequires:	libxml2-devel
Requires:	libvirt
Requires:	php(zend-abi) = %{php_zend_api}
Requires:	php(api) = %{php_core_api}


%description
PHP language bindings for Libvirt API. 
For more details see: http://phplibvirt.cybersales.cz/ http://www.libvirt.org/ http://www.php.net/

%prep
%setup -q -n libvirt-php-%{version}

%build
%configure --with-html-dir=%{_datadir}/doc --with-html-subdir=%{name}-%{version}/html
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_libdir}/php/modules/libvirt-php.so
%{_sysconfdir}/php.d/libvirt-php.ini
%doc %{_datadir}/doc/%{name}-%{version}/html

%changelog
