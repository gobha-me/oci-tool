Name:           oci-sync
Version:        1.0
Release:        1%{?dist}
Summary:        A tool to synchronize OpenShift Container Images from public repositories to self-hosted repositories.

License: BSD-3-Clause
URL: https://github.com/gobha-me/oci-tool/blob/master/README.md
Source0: https://github.com/gobha-me/oci-tool

BuildRequires:
Requires:

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
mkdir -p /usr/bin/
install -m 755 oci-sync /usr/bin/oci-sync
#/bin/
%make_install

%files
%doc
#insert manfiles here

%changelog
* Release
-