#!/bin/bash
#
#

pid=$(ps aux | grep 'hubloadv3-charge-1.0.1.jar' | grep -v grep | wc -l)
echo "$pid"
if [ ! "$pid" -eq 0 ]; then
    echo "program alive";
else
    echo "program stopped";
    #/root/reload_hubload.sh;
fi