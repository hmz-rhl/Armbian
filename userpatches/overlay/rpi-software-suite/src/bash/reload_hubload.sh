#!/bin/bash

# Le script de trouve dans le répertoire /root

if [ -d /root/upgrade_files ]; then
    echo "Une mise à jour à effectuer"
    /opt/hubload/libc/runc.sh stop
    /opt/hubload/java/runjava.sh stop

    mv /root/upgrade_files/* /opt/hubload
    chmod 750 /opt/hubload/libc/*.sh
    chmod 750 /opt/hubload/java/*.sh

    # Etape 2 : Lancement des scripts d'init
    /opt/hubload/libc/runc.sh init
    /opt/hubload/java/runjava.sh init

    rm -rf /root/upgrade_files
    rm -f /root/upgrade.zip

    /opt/hubload/libc/runc.sh start
    /opt/hubload/java/runjava.sh start
else
    echo "Pas de mise à jour"

    /opt/hubload/libc/runc.sh reload
    /opt/hubload/java/runjava.sh reload
fi
