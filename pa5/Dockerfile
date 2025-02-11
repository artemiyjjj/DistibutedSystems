FROM ubuntu:14.04 

ADD . /usr/src/distr

RUN apt-get update \
 && apt-get install -y software-properties-common \
 && add-apt-repository -y ppa:ubuntu-toolchain-r/test
RUN apt-get update \
 && apt-get install -y \
  build-essential \
  libstdc++-6-dev \
  clang-3.5 
RUN apt-get install systemd
RUN apt-get install vim
RUN apt-get install ssh

#  clang -std=c99 -Wall -pedantic *.c –L. -lruntime
