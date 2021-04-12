FROM debian:buster

RUN apt-get update \
 && apt-get install -y valgrind gcc cmake make \
 && apt-get clean
