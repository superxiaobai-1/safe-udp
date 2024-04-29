FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get clean && \
    apt-get autoclean

RUN apt update && \
    apt install -y \
    vim \
    htop \
    apt-utils \
    curl \
    cmake \
    net-tools \
    gdb  gcc g++ \
    libgoogle-glog-dev

WORKDIR /work
 

