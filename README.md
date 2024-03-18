1. to do:
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
# usefull command

* compile and restart cluster
```shell
clear && export TRIPLET=x64-linux-dynamic && cmake -S . -B build/${TRIPLET} -DCMAKE_BUILD_TYPE=Debug -DVCPKG_TARGET_TRIPLET=${TRIPLET} -DCMAKE_TOOLCHAIN_FILE=${HOME}/vcpkg-${TRIPLET}/scripts/buildsystems/vcpkg.cmake && cmake --build build/${TRIPLET} -j $(nproc) && docker compose down && sudo rm -rf tmp && docker build -t spkdfs:latest --build-arg DST=node -f Dockerfiles/Dockerfile.dev ./build/${TRIPLET} && docker compose up -d
```
* compile and not restart cluster
```shell
clear && echo "clibuild" && export TRIPLET=x64-linux-dynamic && cmake -S . -B build/${TRIPLET} -DCMAKE_BUILD_TYPE=Debug -DVCPKG_TARGET_TRIPLET=${TRIPLET} -DCMAKE_TOOLCHAIN_FILE=${HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake && cmake --build build/${TRIPLET} -j $(nproc)
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

# notice
glog has delay, call google::FlushLogFiles(INFO....) after throwing exception 
# comparing with spkdfs before

| filesize | erasure-code, section(MB) | cost time(s) | ref: scp cost time(s)    |
| -------- | ---------------- | --------------- | ------- |
| 100M     | 3+2-64           | 31.918          | 19.031  |
| 1G       | 3+2-64           | 4:30.63         | 2:58.31 |
| 100M     | 7+5-16           | 30.685          | 19.031  |
| 1G       | 7+5-16           | 4:32.16         | 2:58.31 |
