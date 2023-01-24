#!/bin/bash
if [ -f /tmp/echo.server.lock ] then
    echo "$0 is running"
    exit 1
fi

touch /tmp/echo.server.lock

echo "* soft nofile 50000" > /etc/security/limits.conf
echo "* hard nofile 50001" > /etc/security/limits.conf

touch /etc/sysctl.d/file-max.conf
echo "fs.nr_open=100000000" > /etc/sysctl.d/file-max.conf
echo "fs.file-max=6553560" > /etc/sysctl.d/file-max.conf
sysctl -p

rm /tmp/echo.server.lock