Name:           oci-sync
Version:        1.0
Release:        1%{?dist}
Summary:        A tool to synchronize OpenShift Container Images from public repositories to self-hosted repositories.

License: BSD-3-Clause
URL: https://github.com/gobha-me/oci-tool/blob/master/README.md
Source0: https://github.com/gobha-me/oci-tool

BuildRequires:
Requires: bash
BuildArch: noarch

%description
OCI Sync is a tool to synchronize OpenShift Container Images from public repositories to self-hosted
repositories on stand-alone or airgapped networks.  It supports synchronization of multiple processor architectures. The
tool also supports v1 and v2 OCI manifests.

%prep
%setup -q
%build
%configure
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p %{buildroot}/%{_bindir}
install -m 0755 %{name} %{buildroot}/%{_bindir}/%{name}
install -m 444 oci-sync.man /usr/share/man/man1/oci-sync.man

%files
%license LICENSE
%{_bindir}/%{name}

%doc
/usr/share/man/man1/oci-sync.man
%changelog
* Release
-