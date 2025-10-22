#!/bin/bash

cat logo;
echo "*************************         arret des services        *************************"
sudo service hubload_machine stop;
sudo service hubload_display stop;
sudo service hubload_leds stop;
sudo service hubload_rfid stop;
sudo service hubload_network stop;

sudo service hubload_java stop;

echo "*************************     make des utilitaires lib      *************************"
sudo make -C lib/;
# sudo make -C lib/ install;
# sudo make -C lib/ clean;
echo "*************************   make des utilitaires expander   *************************"
# cd ../expander/;
sudo make -C expander/;
sudo make -C expander/ install;
sudo make -C expander/ clean;
echo "*************************   make des utilitaires hardware   *************************"
sudo make -C hardware/;
sudo make -C hardware/ install;
sudo make -C hardware/ clean;

sudo mkdir -p /opt/hubload/java;
sudo cp -r ../../distribution/java/* /opt/hubload/java/;

sudo echo "*************************    copie des fichiers images    *************************"
sudo mkdir -p /opt/hubload/resources/
sudo  cp -r hubload/img/ /opt/hubload/resources/
sudo  cp -r hubload/ttf/ /opt/hubload/resources/

echo "************************* compilation des binaires services *************************"
# Pour compilation sous windows, retirer -lwiringPi :

sudo gcc display.c -c
sudo gcc display.o lib/*.o  -o compiled/hubdisplay -Wall -lSDL2 -lSDL2_ttf -lgpiod -lpthread
sudo gcc leds.c -c
sudo gcc leds.o lib/*.o  -o compiled/hubleds -Wall -lm -lgpiod -lpthread
sudo gcc machine.c -c -lpthread -Wno-unused-variable
sudo gcc machine.o lib/*.o  -o compiled/hubmachine -lpthread -Wall -lm -lgpiod -lpthread -Wno-unused-variable
sudo gcc rfid.c -c
sudo gcc rfid.o lib/*.o  -o compiled/hubrfid -Wall -lm -lgpiod -lpthread
sudo gcc network_tst.c -c
sudo gcc network_tst.o lib/*.o  -o compiled/hubnetwork -Wall -lm -lgpiod -lpthread

echo "*************************    copie des fichiers compilés    *************************"
sudo mkdir -p /opt/hubload/libc/
sudo mkdir -p /usr/share/hubload/notif/
sudo cp compiled/hubmachine /opt/hubload/libc;
sudo cp compiled/hubleds /opt/hubload/libc;
sudo cp compiled/hubrfid /opt/hubload/libc;
sudo cp compiled/hubdisplay /opt/hubload/libc;
sudo cp compiled/hubnetwork /opt/hubload/libc;
sudo cp compiled/startmachine.sh /opt/hubload/libc;

echo "*************************      installation led     *************************"
cp compiled/led_sequence /usr/bin;

echo "*************************      installation des services     *************************"
sudo cp services/* /etc/systemd/system/
sudo cp ../../distribution/java/hubload_java.service /etc/systemd/system/ # a revoir si on centralise les services dans services/
echo "*************************      redémarrage des services     *************************"
sudo service hubload_network enable;
sudo service hubload_display enable;
sudo service hubload_machine enable;
sudo service hubload_leds enable;
sudo service hubload_rfid enable;

sudo service hubload_java enable;

sudo service hubload_network start;
sudo service hubload_display start;
sudo service hubload_machine start;
sudo service hubload_leds start;
sudo service hubload_rfid start;

sudo service hubload_java start;

#sudo reboot;