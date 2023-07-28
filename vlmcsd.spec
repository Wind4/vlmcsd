Summary: 	Microsoft Key Management System       
Name:           vlmcsd
Version:	0.0.1        
Release:        1%{?dist}
License:        NULL
Source:		%{name}-%{version}.tar.gz
URL:            https://github.com/Wind4/vlmcsd
%description
Microsoft Key Management System

%prep
rm -rf $RPM_BUILD_ROOT/*
%setup -q

%install
install -d $RPM_BUILD_ROOT/
cp -a * $RPM_BUILD_ROOT/

%files
%defattr(-,root,root,-) 
%{_prefix}/local/vlmcsd/static/vlmcsd-x64-musl-static
%{_prefix}/local/vlmcsd/vlmcsd.service

%clean
rm -rf $RPM_BUILD_ROOT

%post
ln -s %{_prefix}/local/vlmcsd/static/vlmcsd-x64-musl-static %{_bindir}/vlmcsd
ln -s %{_prefix}/local/vlmcsd/vlmcsd.service %{_prefix}/lib/systemd/system

%preun
systemctl disable vlmcsd.service
systemctl stop vlmcsd.service

%postun
rm -rf /usr/local/vlmcsd
rm -rf /usr/lib/systemd/system/vlmcsd.service
rm -rf /usr/bin/vlmcsd

%changelog
