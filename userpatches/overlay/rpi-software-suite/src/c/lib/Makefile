CC= gcc -Wall
AR= ar -rv
NAME= Hubload
SRC= $(NAME).c
HDR= $(SRC:.c=.h)
OBJ= $(SRC:.c=.o)
LIB= lib$(SRC:.c=).a
DIR=

all:	rtc_eeprom.o	pn532.o	PN532_Rpi_I2C.o	hubfiles.o BSP_wrappers.o
#	echo "On change de repertoire -> /lib";
#	cd ./lib;
	@echo "On genere les bibliothèques statiques";
#	sudo gcc -o lib$(NAME).so -shared $^;
	sudo $(AR) lib$(NAME).a $^;

BSP_wrappers.o:	BSP_wrappers.c
	@echo "generation de l'objet BSP_wrappers...";
	sudo $(CC) -o $@ -c $< -lpthread -lgpiod;

rtc_eeprom.o:	rtc_eeprom.c
	@echo "generation de l'objet rtc_eeprom...";
	sudo $(CC) -o $@ -c $<;

pn532.o:	pn532.c
	@echo "generation de l'objet pn532...";
	sudo $(CC) -o $@ -c $<;

PN532_Rpi_I2C.o:	PN532_Rpi_I2C.c
	@echo "generation de l'objet PN532_Rpi_I2C...";
	sudo $(CC) -o $@ -c $<;

hubfiles.o:	hubfiles.c
	@echo "generation de l'objet hubfiles...";
	sudo $(CC) -o $@ -c $<;

ws2811.o:	ws2811.c
	@echo "generation de l'objet ws2811...";
	sudo $(CC) -o $@ -c $<;

.PHONY: install
install:
	@echo "installation des bibliothèques";
	sudo cp *.a $(DIR)/usr/lib;
	sudo cp *.h $(DIR)/usr/include;
	@echo "l'installation fini";
	@echo "checking des installations";
# cd ../test; 
	sudo gcc ../test/test_lib.c -o ../test/tests -Wall -lHubload;
	sudo ../test/tests;
# cd ../;

.PHONY: uninstall
uninstall:
	sudo rm -f $(DIR)/usr/lib/libHubload.*;
	sudo rm -f $(DIR)/usr/include/expander_i2c.h $(DIR)/usr/include/MCP3202.h $(DIR)/usr/include/rtc_eeprom.h $(DIR)/usr/include/pn532.h $(DIR)/usr/include/PN532_Rpi_I2C.h;
	sudo rm -f $(DIR)/usr/include/hubfiles.h $(DIR)/usr/include/ws2811.h;

.PHONY: clean
clean:
	sudo rm -f *.o *.a;
