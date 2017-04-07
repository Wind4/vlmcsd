%define use_openssl 0

Name:		vlmcsd
Version:	1107
Release:	1
Summary:	KMS Emulator in C
License:	WTFPL
Group:		System/Daemons
Url:		https://forums.mydigitallife.info/threads/50234-Emulated-KMS-Servers-on-non-Windows-platforms
Source0:	%{name}-svn%{version}.tar.gz
Source1:	%{name}.service
BuildRequires:	gmake
BuildRoot:	%{_tmppath}/%{name}-%{version}-build

%description
vlmcsd - portable open-source KMS Emulator in C
vlmcsd is
- a replacement for Microsoft's KMS server
- It contains vlmcs, a KMS test client, mainly for debugging purposes, that also can "charge" a genuine KMS server
- designed to run on an always-on or often-on device, e.g. router, NAS Box, ...
- intended to help people who lost activation of their legally-owned licenses, e.g. due to a change of hardware (motherboard, CPU, ...)
vlmcsd is not
- a one-click activation or crack tool
- intended to activate illegal copies of software (Windows, Office, Project, Visio)

%prep
%setup -q -n %{name}-svn%{version}

%build
%if %{use_openssl} == 1
gmake CRYPTO=openssl all
gmake CRYPTO=openssl libkms
gmake CRYPTO=openssl libkms-static
%else
gmake all
gmake libkms
gmake libkms-static
%endif

%install
#gmake DESTDIR=%{buildroot} install
mkdir -p %{buildroot}%{_bindir}
install -m 0755 bin/vlmcs %{buildroot}%{_bindir}/
install -m 0755 bin/vlmcsd %{buildroot}%{_bindir}/

mkdir -p %{buildroot}%{_libdir}
install -m 0755 lib/libkms.so %{buildroot}%{_libdir}/
install -m 0755 lib/libkms.a %{buildroot}%{_libdir}/

mkdir -p %{buildroot}%{_sysconfdir}/%{name}
install -m 644 etc/vlmcsd.ini %{buildroot}%{_sysconfdir}/%{name}/
install -m 644 etc/vlmcsd.kmd %{buildroot}%{_sysconfdir}/%{name}/

mkdir -p %{buildroot}%{_mandir}/man{1,5,7,8}
gzip -c man/vlmcs.1 > %{buildroot}%{_mandir}/man1/vlmcs.1.gz
gzip -c man/vlmcsd.7 > %{buildroot}%{_mandir}/man7/vlmcsd.7.gz
gzip -c man/vlmcsd.8 > %{buildroot}%{_mandir}/man8/vlmcsd.8.gz
gzip -c man/vlmcsd-floppy.7 > %{buildroot}%{_mandir}/man7/vlmcsd-floppy.7.gz
gzip -c man/vlmcsd.ini.5 > %{buildroot}%{_mandir}/man5/vlmcsd.ini.5.gz
gzip -c man/vlmcsdmulti.1 > %{buildroot}%{_mandir}/man1/vlmcsdmulti.1.gz

mkdir -p %{buildroot}%{_datadir}/%{name}
install -m 644 floppy/floppy144.vfd %{buildroot}%{_datadir}/%{name}/

mkdir -p %{buildroot}%{_unitdir}
install -m 644 %{SOURCE1} %{buildroot}%{_unitdir}/

%post
%systemd_post %{name}.service

%preun
%systemd_preun %{name}.service

%postun
%systemd_postun %{name}.service

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%{_bindir}/vlmcs
%{_bindir}/vlmcsd
%dir %{_sysconfdir}/%{name}
%config %{_sysconfdir}/%{name}/vlmcsd.ini
%{_sysconfdir}/%{name}/vlmcsd.kmd
%{_unitdir}/%{name}.service
%{_datadir}/%{name}/floppy144.vfd
%{_libdir}/libkms.so
%{_libdir}/libkms.a
%{_mandir}/man?/*


%changelog
