FROM ubuntu:22.04
RUN sed -i "s@http://.*archive.ubuntu.com@http://mirrors.cqu.edu.cn@g" /etc/apt/sources.list && \
    sed -i "s@http://.*security.ubuntu.com@http://mirrors.cqu.edu.cn@g" /etc/apt/sources.list && \
    apt update && apt install -y iputils-ping curl masscan libpcap0.8 gdb libdw-dev libbfd-dev libdwarf-dev
RUN mkdir /spkdfs && mkdir /spkdfs/bin && mkdir /spkdfs/data && mkdir /spkdfs/logs &&  mkdir /spkdfs/coredumps
COPY ./build/node /spkdfs/bin/node
ENTRYPOINT ["/spkdfs/bin/node"]