Driver Hubload + mise à jour
============================================

Le répertoire distribution contient la copie du répertoire qui sera déployé dans /opt/hubload sur la borne

On génère un zip nommé num_version.zip qui contuent les fichiers dans le répertoire distribution
Ex : v0_0_8_2022_07_03.zip

A la borne, on lui donne comme URL du firmware :
https://jerecharge.com/api/admin/firmware/{num_version}

Commandes utiles :
1. Copier le contenu d'une carte SD dans un fichier :
dd if=/dev/sda of=hubload_v2.x.x.img bs=1024k status=progress

2. Copier une image sur une carte SD :
dd if=hubload_v2.x.x.img of=/dev/sdb bs=1024k status=progress

3. Compresser une image avec le suivi du statut :
tar cf - hubload_v2.x.x.img | pv -s $(du -sb hubload_v2.x.x.img | awk '{print $1}') | gzip > hubload_v2.x.x.tar.gz

4. Monter une image pour vérifier / modifier son contenu
sudo modprobe loop
sudo losetup -f       -> devrait retourner l'adresse /dev/loop0
sudo losetup /dev/loop0 hubload_v2.x.x.img
sudo partprobe /dev/loop0     -> A ce stade, on doit retrouver /dev/loop0p1 et /dev/loop0p2
Note : pour démonter : sudo losetup -d /dev/loop0

5. Redimensionner une image
sudo gparted /dev/loop0      -> On peut alors réduire la taille de l'image
sudo losetup -d /dev/loop0
fdisk -l hubload_v2.x.x.img   -> On repère Z, l'adresse de fin de la 2nde partition
truncate --size=$[(Z+1)*512] hubload_v2.x.x.img

6. Modifier une image (une fois montée à l'étape 4) :
sudo mount /dev/loop0p1 /media/hubload/boot
sudo mount /dev/loop0p2 /media/hubload/rootfs
Pour démonter :
sudo umount /dev/loop0p1
sudo umount /dev/loop0p2