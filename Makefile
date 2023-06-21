export XDG_CONFIG_HOME := $(shell pwd)/XDG_CONFIG_HOME
export XDG_DATA_HOME := $(shell pwd)/XDG_DATA_HOME

all: clean compile test

clean:
	#rm -rf XDG_DATA_HOME/welcome-home
	rm -f *.o

compile:
	gcc main.c xdg.c data.c terminal.c -o main.o

test:
	./main.o
