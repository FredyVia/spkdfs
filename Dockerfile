FROM ubuntu:22.04
RUN mkdir /spkdfs && mkdir /spkdfs/bin && mkdir /spkdfs/data && mkdir /spkdfs/logs 
COPY ./build/node /spkdfs/bin/node
ENTRYPOINT ["/spkdfs/bin/node"]