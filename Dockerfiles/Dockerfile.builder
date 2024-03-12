FROM ubuntu:22.04
ENV ENV_FILE="/env.sh"
RUN apt update && apt install -y --no-install-recommends curl zip unzip tar pkg-config flex bison autoconf libtool automake linux-libc-dev python3 ca-certificates cmake git build-essential && apt clean;