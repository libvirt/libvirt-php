Name:		php-libvirt
Version:	0.4
Release:	1%{?dist}
Summary:	PHP language binding for Libvirt
Group:		Development/Libraries
License:	PHP
URL:		http://phplibvirt.cybersales.cz/
Source0:	http://phplibvirt.cybersales.cz/php-libvirt-%{version}.tar.gz
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
%setup -q -n php-libvirt-%{version}
phpize

%build
%configure
./configure --enable-libvirt
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot} INSTALL_ROOT=%{buildroot}
mkdir -p "%{buildroot}%{_defaultdocdir}/php-libvirt/"
cp -r doc "%{buildroot}%{_defaultdocdir}/php-libvirt/"
mkdir -p "%{buildroot}%{_sysconfdir}/php.d/"
echo -e "; Enable libvirt extension module\nextension=libvirt.so" > "%{buildroot}%{_sysconfdir}/php.d/libvirt.ini"

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_libdir}/php/modules/libvirt.so
%{_sysconfdir}/php.d/libvirt.ini
%doc
%{_defaultdocdir}/php-libvirt/

%changelog
