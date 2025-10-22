#!/bin/bash

# Le script de trouve dans le répertoire /root

# Etape 0 : On vérifie si les répertoires /opt/hubload et /usr/share/hubload existent
if [ ! -d /opt/hubload ]; then
    mkdir /opt/hubload
fi

if [ ! -d /usr/share/hubload ]; then
   mkdir /usr/share/hubload
fi

# On vérifie s'il y a des fichiers dans /root/upgrade_files
# S'il y a des fichiers, on vide le répertoire /opt/hubload et on le remplace par le contenu de /root/upgrade_files
if [ -d /root/upgrade_files ]; then
    echo "Une mise à jour à effectuer"

    service hubloadv3 stop
    service hubload_c stop

    # On installe les fichiers dans leur destination
    rm -rf /opt/hubload/*
    rm -f /var/log/hubload*.log

    mv /root/upgrade_files/* /opt/hubload
    chmod -R 750 /opt/hubload

    # Lancement des scripts d'init
    /opt/hubload/libc/runc.sh init
    /opt/hubload/java/runjava.sh init

    systemctl daemon-reload

    # On supprimer
    rm -rf /root/upgrade_files
    rm -f /root/upgrade.zip

    # Etape 3 : Lancement des services
    /opt/hubload/libc/runc.sh restart
    /opt/hubload/java/runjava.sh restart
else
    echo "Pas de mise à jour"
fi
