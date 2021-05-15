#!/bin/sh

set -e
umask 22

mkdir -p /usr/src/ucb/rlogin
cp -p /usr/src/ucb/rlogin.c /usr/src/ucb/rlogin/rlogin.c
cp -p /usr/src/man/man1/rlogin.1 /usr/src/ucb/rlogin/rlogin.1
