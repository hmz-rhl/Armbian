#!/bin/bash  

# Le script de trouve dans le répertoire /root  

if [ -d /root/upgrade_files ]; then
   echo "Une mise à jour à effectuer"     
   
   FILE=/root/upgrade_files/upgrade.sh 
   if [ -f "$FILE" ]; then     
      echo "$FILE exists." 
      chmod 750 "$FILE"
      "$FILE"
   else     
      echo "$FILE does not exist." 
   fi
   
    rm -rf /root/upgrade_files     
    rm -f /root/upgrade.zip 

   reboot    
else     
    echo "Pas de mise à jour"      
    echo "Off" > /usr/share/hubload/notif/STATE_MACHINE
    /opt/hubload/libc/hubmachine
fi
