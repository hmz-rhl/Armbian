#!/bin/bash

function  init() {
    cp /opt/hubload/java/hubloadv3.service /etc/systemd/system;
    systemctl daemon-reload;
    systemctl enable hubloadv3;
}

function reload() {
    systemctl reload hubloadv3;
#    if [ $(ps aux | grep 'hubloadv3-charge-0.9.1.jar' | grep -v grep | wc -l -eq 1 ]; then
#        kill $(ps aux | grep 'hubloadv3-charge-0.9.1.jar' | grep -v grep | awk '{print $2}');
#    fi
#    java \
#        -Degl.displayid=/dev/dri/card0 \
#        --module-path /home/pi/install/javafx-sdk-18/lib \
#        --add-modules javafx.controls,javafx.graphics,javafx.fxml \
#        -Dprism.verbose=true \
#        -Dembedded=monocle \
#        -Dglass.platform=Monocle \
#        -classpath '/opt/hubload/java/hubloadv3-charge-0.9.1.jar:/opt/pi4j/lib/*' \
#        com.saemload.charging.ChargingScreen;
}

function start() {
    systemctl start hubloadv3;
#    java \
#        -Degl.displayid=/dev/dri/card0 \
#        --module-path /home/pi/install/javafx-sdk-18/lib \
#        --add-modules javafx.controls,javafx.graphics,javafx.fxml \
#        -Dprism.verbose=true \
#        -Dembedded=monocle \
#        -Dglass.platform=Monocle \
#        -classpath '/opt/hubload/java/hubloadv3-charge-0.9.1.jar:/opt/pi4j/lib/*' \
#        com.saemload.charging.ChargingScreen;
}

function stop() {
    systemctl stop hubloadv3;
#    kill $(ps aux | grep 'hubloadv3-charge-0.9.1.jar' | grep -v grep | awk '{print $2}');
}

if [ "$#" -eq 1 ]; then 
    case "$1" in 
        "init") init ;; 
        "reload") reload ;; 
        "start") start ;; 
        "stop") stop ;; 
        *) 
            echo -e "Commande $1 non reconnue.";; 
    esac
else 
    echo -e "Aucune commande passée en paramètre";
fi 


