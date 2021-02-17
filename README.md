### WHY
This spawned from the need of doing a multi arch sync, which requires a multi arch copy
 and learn how REST interfaces can be implemented, so going the 'hard' route was a personal choice

### DEPENDENCIES
On RHEL 7 enable rhel-server-rhscl-7-rpms repo and install devtoolset-9-gcc-c++, ensure to 
```
$ scl enable devtoolset-9 bash
```
prior to running a build.  The Tool Set is also required for a runtime environment for the version of libstdc++.

### BUILD
Release build
```
$ mkdir BUILD
$ cd BUILD
$ cmake ..
$ make
```
Debug Build
```
$ mkdir BUILD
$ cd BUILD
$ cmake -DCMAKE_BUILD_TYPE=Debug ..
$ make
```

### EXAMPLES
```
oci-sync --help
oci-sync -vl err.log yaml:<yaml-file> dir:<path>
oci-sync yaml:<yaml-file> docker://<domain|ip>
oci-sync dir:<path>/<domain> docker://<domain|ip>
```

### CLANG-TIDY
```
clang-tidy src/bin/oci-sync.cpp -- -Iinclude --std=c++17
```

On the TODO list

- Implement Image and Manifest DELETE - this is more important for Dir than Client which may be noop
- Implement Manifest v2 Schema 1 methods for images that do not have the modern Schema 2 manifests
- Source and Destination is very seperate, this disallows at the moment doing a resume (per the API) on either side
  - There are senarios where this is a problem, it is about timing and the timeout period for an upload on the registry side
- Tests???? (Being Added)
  - still not sure how to write good tests for this, as it is heavily depentant on a properly implemented docker registry
- spdlog has replaced std::cout and std::cerr, next step is to determine how to handle exceptions and weither custome exceptions are needed
