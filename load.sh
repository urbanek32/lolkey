#!/bin/bash

INSMOD=`which insmod`
MODULE=lolkey

echo "Loading module..."
$INSMOD $MODULE.ko || exit 1
echo "Read log through cat /root/log.txt"