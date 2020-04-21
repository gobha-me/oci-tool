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
- Tests????
