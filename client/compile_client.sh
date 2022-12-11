#!/bin/sh

gcc client.c ../common/tftp.c -o client
chmod 744 client