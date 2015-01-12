#!/bin/bash

RMMOD=`which rmmod`
MODULE=lolkey

echo "Removing module..."
$RMMOD $MODULE.ko || exit 1