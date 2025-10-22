CC= gcc -Wall
AR= ar -rv
NAME= Expander
SRC= $(NAME).c
HDR= $(SRC:.c=.h)
OBJ= $(SRC:.c=.o)
LIB= lib$(SRC:.c=).a
DIR=
LDFLAGS=-lgpiod

all:	expander-get-id expander-set-id expander-get-wh expander-add-wh
	echo "On genere les binaire expander";
#	cd ./lib;
# 	sudo gcc expander-get-id.o ../lib/*.o -o expander-get-id -Wall;
# 	sudo gcc expander-set-id.o ../lib/*.o -o expander-set-id -Wall -lm;
# 	sudo gcc expander-add-wh.o ../lib/*.o -o expander-add-wh -Wall;
# 	sudo gcc expander-get-wh.o ../lib/rtc_eeprom.o -o expander-get-wh -Wall -lm;
#	make clean;

expander-get-id:	expander-get-id.o ../lib/*.o
	@echo "generation de expander-get-id...";
	sudo $(CC) -o $@ $^ $(LDFLAGS);

expander-set-id:	expander-set-id.o ../lib/*.o
	@echo "generation de expander-set-id...";
	sudo $(CC) -o $@ $^ $(LDFLAGS);

expander-get-wh:	expander-get-wh.o ../lib/*.o
	@echo "generation de expander-get-wh...";
	sudo $(CC) -o $@ $^ $(LDFLAGS);

expander-add-wh:	expander-add-wh.o ../lib/*.o
	@echo "generation de expander-add-wh...";
	sudo $(CC) -o $@ $^ $(LDFLAGS);

%.o:	%.c
	@echo "generation des objets expander";
	sudo $(CC) -o $@ -c $^;

.PHONY: install
install:
	@echo "installation des commandes Expander";
	sudo cp expander-get-id expander-set-id expander-get-wh expander-add-wh /usr/bin;
	@echo "checking des installations";
# cd ../test; 
# expander-get-gpio 26;
# expander-set-gpio -h;
# cd ../;

.PHONY: uninstall
uninstall:
	sudo rm -f /usr/bin/expander-get-id /usr/bin/expander-set-id /usr/bin/expander-get-wh /usr/bin/expander-add-wh;

.PHONY: clean
clean:
	sudo rm -f *.o *.a expander-get-id expander-set-id expander-get-wh expander-add-wh
