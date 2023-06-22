export XDG_CONFIG_HOME := $(shell pwd)/XDG_CONFIG_HOME
export XDG_DATA_HOME := $(shell pwd)/XDG_DATA_HOME

all: clean compile

clean:
	rm -f XDG_DATA_HOME/welcome-home/.data
	rm -f *.o

compile:
	gcc main.c xdg.c data.c terminal.c -o main.o

debug:
	./main.o -d
