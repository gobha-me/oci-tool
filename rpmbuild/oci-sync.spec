Name:           oci-sync
Version:        1.0
Release:        1.0
Summary:        A tool to synchronize OpenShift Container Images from public repositories to self-hosted repositories.

License: BSD-3-Clause
URL: https://github.com/gobha-me/oci-tool
Source0: oci-sync

Requires: bash
BuildArch: x86_64

%description
OCI Sync is a tool to synchronize OpenShift Container Images from public repositories to self-hosted
repositories on stand-alone or airgapped networks.  It supports synchronization of multiple processor architectures. The
tool also supports v1 and v2 OCI manifests.

%prep
%setup


%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_mandir}/man1
mkdir -p %{buildroot}%{_docdir}
cp -r /home/gitlabRunner/rpmbuild/SOURCES/oci-sync.bin %{buildroot}%{_bindir}/oci-sync
cp -r /home/gitlabRunner/rpmbuild/SOURCES/oci-sync.man %{buildroot}%{_mandir}/man1/
cp -r /home/gitlabRunner/rpmbuild/SOURCES/LICENSE %{buildroot}%{_docdir}/

%files
%{_bindir}/%{name}

%doc /usr/share/man/man1/oci-sync.man.gz
%doc %name-%version/LICENSE
%changelog
* Fri Mar 05 2021 Jeff Smith <jefsmith@redhat.com> Initial Release
-