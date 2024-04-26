FROM ubuntu:22.04

RUN apt update && \
    apt -y install wget git make

RUN wget https://sophon-file.sophon.cn/sophon-prod-s3/drive/23/03/07/16/host-tools.tar.gz && \
    tar xvf host-tools.tar.gz
ENV PATH=$PATH:/host-tools/gcc/riscv64-linux-musl-x86_64/bin
