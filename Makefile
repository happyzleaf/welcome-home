ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

all: clean compile

clean:
	rm -f XDG_DATA_HOME/welcome-home/.data
	rm -f main.o

compile:
	gcc main.c xdg.c data.c terminal.c -o main.o

install:
	cp main.o $(DESTDIR)$(PREFIX)/bin/welcome-home

debug:
	export XDG_CONFIG_HOME=$(shell pwd)/XDG_CONFIG_HOME
	export XDG_DATA_HOME=$(shell pwd)/XDG_DATA_HOME
	./welcome-home -a -d
