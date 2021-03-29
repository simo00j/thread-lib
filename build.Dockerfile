FROM alpine:latest

RUN apk add --no-cache valgrind gcc cmake make musl-dev
