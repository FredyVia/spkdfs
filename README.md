# To do:

  * update_lock add failinfo and update_lock in one apply
  * inode lock time sync
  * seprate CMakeLists.txt
  * test
  * inode sub may be very long long long
  * sdk inode cache based on time
  * remove callbacks
  * solve the situation of killing 40% of nodes at once and losing meta data (by using exponential recovery)
  * dynamic node management
  * dynamic block repair
  * guess storage by RS<RE<>>
  * RS not support 12+8

# depends & build

ref Dockerfiles/
1. Dockerfile.builder
1. Dockerfile.runtime
1. Dockerfile.node
1. Dockerfile.dev
# usefull command

* compile and restart cluster
```shell
clear && export TRIPLET=x64-linux-dynamic && cmake -S . -B build/${TRIPLET} -DCMAKE_BUILD_TYPE=Debug -DVCPKG_TARGET_TRIPLET=${TRIPLET} -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake && cmake --build build/${TRIPLET} -j $(nproc) && docker compose down && sudo rm -rf ./tmp && docker build -t spkdfs:latest -f Dockerfiles/Dockerfile.dev . && docker compose up -d
```
* compile and not restart cluster
```shell
clear && echo "clibuild" && export TRIPLET=x64-linux-dynamic && cmake -S . -B build/${TRIPLET} -DCMAKE_BUILD_TYPE=Debug -DVCPKG_TARGET_TRIPLET=${TRIPLET} -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake && cmake --build build/${TRIPLET} -j $(nproc)
```
* mkdir
```shell
build/x64-linux-dynamic/client -datanode=192.168.88.112:8001 -command=mkdir /$(uuidgen)
```
* ls
```shell
build/x64-linux-dynamic/client -command=ls / -datanode=192.168.88.112:8001
```
* rmdir & rm
```shell
build/x64-linux-dynamic/client -command=rm /file_or_directory -datanode=192.168.88.112:8001
```
* upload(using erasure code requires package libjerasure-dev)
```shell
build/x64-linux-dynamic/client -datanode="192.168.88.112:8001" -command=put -storage_type="RS<7,5,16>" /source_in_client /xxxtmp.log
```
* download
```shell
build/amd64/client -datanode="192.168.88.109:8001" -command=get /source_in_server /dst
```

# complie commands (new)

```shell
# with Makefile
make # compile all source code
make test # compile tests, the same below
make fuse
make benchmark
make clean # rm -rf build

# without Makefile
# compile with target's build-flag turning on, e.g. -DBUILD_TEST:BOOL=ON/OFF
export TRIPLET=x64-linux-dynamic && cmake -B build/${TRIPLET} -DCMAKE_BUILD_TYPE=Debug -DVCPKG_TARGET_TRIPLET=${TRIPLET} -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake -DBUILD_BENCHMARK:BOOL=ON -DBUILD_TEST:BOOL=ON -DBUILD_FUSE:BOOL=ON && cmake --build build/${TRIPLET} -j $(nproc)
```

# notice
glog has delay, call google::FlushLogFiles(INFO....) after throwing exception.

# SDK Lock

* remote lock: lock write, not lock read

* local lock: logic_lock, real_lock

  > logic_lock(for Inode, In a scenario where the same file is opened multiple times and the second time the server retrieves the inode, it is found that it has changed. At this time, it is necessary to update the local inode. At this time, the first open has not yet closed, and hastily replacing the inode with a new one may cause consistency issues. It is possible that the content read from this file contains both previous and latest data. If the server does not lock it, this situation cannot be avoided, but we should also do our best.), 

* real_lock(for file write, align granularity)

  >  The locking of local cache files, more precisely, is the locking of a slice of the file. When the upper layer application reads from a buffer smaller than our block size, it will cause the client to pull the same data block from the datanode multiple times. When preparing to pull, the lock will be applied, and when the pull is completed, it will be released. When the upper layer application requests different data blocks during the pull, but the data block is within the same slice, it will cause subsequent requests to block and wait for the first pull to complete.

# bench on raspberrypi 4B previous spkdfs

| filesize | erasure-code, section(MB) | cost time(s) | ref: scp cost time(s)    |
| -------- | ---------------- | --------------- | ------- |
| 100M     | 3+2-64           | 31.918          | 19.031  |
| 1G       | 3+2-64           | 4:30.63         | 2:58.31 |
| 100M     | 7+5-16           | 30.685          | 19.031  |
| 1G       | 7+5-16           | 4:32.16         | 2:58.31 |
