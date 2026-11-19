FROM ubuntu:20.04

RUN apt-get update 
RUN apt-get install -y wget make bzip2 python3

RUN cd /opt && \
	wget http://shyboy.oss-cn-shenzhen.aliyuncs.com/readonly/tc32_gcc_v2.0.tar.bz2 && \
	tar jxvf tc32_gcc_v2.0.tar.bz2

WORKDIR /app

ENV TC32_HOME="/opt/tc32" \
	  PATH="/opt/tc32/bin:${PATH}"