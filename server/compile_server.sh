#!/bin/sh

gcc -pthread server.c ../common/tftp.c -o server
chmod 744 server