CC=gcc
CFLAGS= -lgattlib
all: victron-smartshunt

victron-smartshunt: victron-smartshunt.c
	$(CC) victron-smartshunt.c $(CFLAGS) -o victron-smartshunt
  
install:
	sudo cp victron-smartshunt /usr/local/bin/

uninstall:
	sudo rm -f /usr/local/bin/victron-smartshunt

clean:
	rm -f victron-smartshunt
	rm -f *.o
