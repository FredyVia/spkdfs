FROM --platform=$BUILDPLATFORM ubuntu:22.04 as BUILDER
ARG TARGETPLATFORM
ARG TARGETARCH
ENV ENV_FILE="/etc/profile.d/arch.sh"
RUN if [ "$TARGETPLATFORM" = "linux/amd64" ]; then \
  echo 'export COMPILER=/usr/bin/x86_64-linux-gnu-g++' >> ${ENV_FILE} && \
  echo 'export TRIPLET=x64-linux-release' >> ${ENV_FILE}; \
  elif [ "$TARGETPLATFORM" = "linux/arm64" ]; then \
  echo 'export COMPILER=/usr/bin/aarch64-linux-gnu-g++' >> ${ENV_FILE} && \
  echo 'export TRIPLET=arm64-linux-release' >> ${ENV_FILE}; \
  else \
  echo "Unsupported architecture: $TARGETPLATFORM" && exit 1; \
  fi && \
  echo 'export PACKAGES="breakpad gtest gflags braft brpc nlohmann-json protobuf glog rocksdb cryptopp liberasurecode libfuse dbg-macro"' >> ${ENV_FILE}

RUN apt update && apt install -y --no-install-recommends curl zip unzip tar pkg-config flex bison autoconf libtool automake linux-libc-dev python3 ca-certificates cmake git build-essential crossbuild-essential-${TARGETARCH} && apt clean;
RUN git clone https://github.com/FredyVia/vcpkg -b libfuse-cpp --depth 1 ~/vcpkg && ~/vcpkg/bootstrap-vcpkg.sh
RUN . ${ENV_FILE} && ~/vcpkg/vcpkg install ${PACKAGES} --triplet=${TRIPLET}
COPY . "/spkdfs"
WORKDIR "/spkdfs"
RUN . ${ENV_FILE} && cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DVCPKG_TARGET_TRIPLET=${TRIPLET} \
  -DCMAKE_CXX_COMPILER=${COMPILER} \
  -DCMAKE_TOOLCHAIN_FILE=${HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake && \
  cmake --build build -j $(nproc)

FROM ubuntu:22.04
RUN mkdir /spkdfs && mkdir /spkdfs/bin && mkdir /spkdfs/data && mkdir /spkdfs/logs &&  mkdir /spkdfs/coredumps
COPY --from=BUILDER /spkdfs/build/node /spkdfs/bin/node
ENTRYPOINT ["/spkdfs/bin/node"]