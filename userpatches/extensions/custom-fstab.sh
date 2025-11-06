# pour ajouter un point de montage en ddr pour le dossier des notifications
function format_partitions__fstab() {
    echo "tmpfs /usr/share/hubload/notif/ tmpfs defaults,nosuid 0 0" >> $SDCARD/etc/fstab
    echo "/etc/fstab updated !"
}

# juste pour tester
function run_after_build__say_congratulations() { 
  echo "Congrats, the build is finished!"
} 
