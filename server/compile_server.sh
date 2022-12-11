#!/bin/sh

gcc -pthread server.c -o server
chmod 744 server