CC= gcc -Wall
AR= ar -rv
NAME= Hubload Hardware
SRC= $(NAME).c
HDR= $(SRC:.c=.h)
OBJ= $(SRC:.c=.o)
LIB= lib$(SRC:.c=).a
DIR=
LDFLAGS=-lgpiod -lm

all:	scan_rfid read_cp_pp read_temp file_events_scan
	echo "On genere les binaire hardware";
#	cd ./lib;
#	sudo gcc led_sequence.o -o led_sequence -Wall -lm -lgpiod -lpthread;
# 	@echo "generation des binaires hardware/ ...";
# 	sudo gcc scan_rfid.o -o scan_rfid -Wall -lm -lgpiod -lpthread;
# 	sudo gcc read_temp.o -o read_temp -Wall -lm -lgpiod -lpthread;
# 	sudo gcc read_cp_pp.o -o read_cp_pp -Wall -lm -lgpiod -lpthread;
# 	sudo gcc file_events_scan.o -o file_events_scan -Wall -Wno-unused-variable -lm -lgpiod -lpthread;
#	make clean;

scan_rfid:	scan_rfid.o ../lib/*.o
	@echo "generation de scan_rfid...";
	sudo $(CC) -o $@ $^ $(LDFLAGS);	

# led_sequence.o:	led_sequence.c
# 	@echo "generation de l'objet led_sequence...";
# 	sudo $(CC) -o $@ -c $^;

read_cp_pp:	read_cp_pp.o ../lib/*.o
	@echo "generation de read_cp_pp...";
	sudo $(CC) -o $@ $^ $(LDFLAGS);

read_temp:	read_temp.o ../lib/*.o
	@echo "generation de read_temp...";
	sudo $(CC) -o $@ $^ $(LDFLAGS);

file_events_scan:	file_events_scan.o ../lib/*.o
	@echo "generation de file_events_scan...";
	sudo $(CC) -o $@ $^ $(LDFLAGS);

%.o:	%.c
	@echo "generation des objets hardware";
	sudo $(CC) -o $@ -c $^;

.PHONY: install
install:
	@echo "installation des commandes Hubload Hardware";
	sudo cp scan_rfid read_cp_pp read_temp file_events_scan /usr/bin;
	@echo "checking des installations";
# cd ../test; 
# sudo led_sequence 50 0 0 0 0;
# cd ../;

.PHONY: uninstall
uninstall:
	sudo rm -f /usr/bin/scan_rfid /usr/bin/read_cp_pp /usr/bin/read_temp /usr/bin/file_events_scan;

.PHONY: clean
clean:
	sudo rm -f *.o *.a scan_rfid read_cp_pp read_temp file_events_scan