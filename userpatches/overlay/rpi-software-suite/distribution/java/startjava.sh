#!/bin/bash  

echo "Lancement application JAVA"

sudo cp /usr/share/hubload/id.conf /usr/share/hubload/notif/VALUE_ID_EVSE
sudo cp /usr/share/hubload/version.conf /usr/share/hubload/notif/VALUE_SW_EVSE

echo "-1" > /usr/share/hubload/notif/STATE_CONNECTIVITY
echo "Off" > /usr/share/hubload/notif/STATE_JAVA

java -jar /opt/hubload/java/hubload-runner.jar