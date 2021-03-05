Name:           oci-sync
Version:        1.0
Release:        1.0
Summary:        A tool to synchronize OpenShift Container Images from public repositories to self-hosted repositories.

License: BSD-3-Clause
URL: https://github.com/gobha-me/oci-tool/blob/master/README.md
Source0: oci-sync
Source1: https://github.com/gobha-me/oci-tool

Requires: bash
BuildArch: noarch

%description
OCI Sync is a tool to synchronize OpenShift Container Images from public repositories to self-hosted
repositories on stand-alone or airgapped networks.  It supports synchronization of multiple processor architectures. The
tool also supports v1 and v2 OCI manifests.

%configure
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p %{buildroot}/%{_bindir}
install -m 0755 %{_name} %{buildroot}/%{_bindir}/%{_name}
install -m 444 %{_name}.man /usr/share/man/man1/%{_name}.man

%files
%license LICENSE
%{_bindir}/%{name}

%doc
/usr/share/man/man1/oci-sync.man
%changelog
* Fri Mar 05 2021 Jeff Smith <jefsmith@redhat.com> Initial Release
-