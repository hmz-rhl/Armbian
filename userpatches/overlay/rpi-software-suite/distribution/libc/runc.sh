#!/bin/bash

function  init() {
    echo "c.init";
    cp /opt/hubload/libc/hubload_c.service /etc/systemd/system;
    systemctl daemon-reload;
    systemctl enable hubload_c;
}

function reload() {
    echo "c.reload";
    systemctl reload hubload_c;
}

function start() {
    echo "c.start";
    systemctl start hubload_c;
}

function stop() {
    echo "c.stop";
    systemctl stop hubload_c;
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