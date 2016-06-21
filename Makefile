PREFIX ?=/usr/local
CC ?= gcc 
CFLAGS = -Wall -I./include

all: midilcm

install : midilcm
	install -m 0755 midilcm $(PREFIX)/bin

uninstall : $(PREFIX)/bin/midilcm
	rm -f $(PREFIX)/bin/midilcm

midilcm: midilcm.c dialcm.h
	$(CC) $(CFLAGS) midilcm.c -o midilcm

clean :
	rm -f midilcm
