### DEPENDACIES
Attempted to have this only require few dependacies (libraries) and be mostly header only for external stuff.
This failed with SHA256 digest validation.  If someone knows of a good one, that works I am all ears. until then
ensure libbotan-2 (dev) is installed.

### BUILD
Building is simple, quick and dirty, spent to much time trying to get cmake to work for this simple build.  Will circle back when have the time
```
./Build.sh
```

### CLANG-TIDY
```
clang-tidy src/bin/oci-sync.cpp -- -Iinclude --std=c++17
```

On the TODO list

- Argument parsing and validation
- Picking and implementing an error/logging solution, to throw or not to throw
- Implement Manifest v2 Schema 1 methods for images that do not have the modern Schema 2 manifests
- Implement asynchronous copies blobs/manifests
- Source and Destination is very seperate, this disallows at the moment doing a resume (per the API) on either side
- Tests????
