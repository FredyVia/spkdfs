.DEFAULT_GOAL := all

TRIPLET := x64-linux-dynamic
CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=Debug -DVCPKG_TARGET_TRIPLET=${TRIPLET} -DCMAKE_TOOLCHAIN_FILE=${HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake

.PHONY: test
test:
	cmake -B build/${TRIPLET} ${CMAKE_FLAGS} -DBUILD_TEST:BOOL=ON -DBUILD_FUSE:BOOL=OFF -DBUILD_BENCHMARK:BOOL=OFF
	cmake --build build/${TRIPLET} -j $(nproc)

.PHONY: fuse
fuse:
	cmake -B build/${TRIPLET} ${CMAKE_FLAGS} -DBUILD_TEST:BOOL=OFF -DBUILD_FUSE:BOOL=ON -DBUILD_BENCHMARK:BOOL=OFF
	cmake --build build/${TRIPLET} -j $(nproc)

.PHONY: benchmark
benchmark:
	cmake -B build/${TRIPLET} ${CMAKE_FLAGS} -DBUILD_TEST:BOOL=OFF -DBUILD_FUSE:BOOL=OFF -DBUILD_BENCHMARK:BOOL=ON
	cmake --build build/${TRIPLET} -j $(nproc)

.PHONY: all
all:
	cmake -B build/${TRIPLET} ${CMAKE_FLAGS} -DBUILD_BENCHMARK:BOOL=ON -DBUILD_TEST:BOOL=ON -DBUILD_FUSE:BOOL=ON
	cmake --build build/${TRIPLET} -j $(nproc)

.PHONY: clean
clean:
	rm -rf build