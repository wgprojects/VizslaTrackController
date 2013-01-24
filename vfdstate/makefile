default: vfdcomm

vfdcomm.o: main.c 
	gcc -c main.c -o vfdcomm.o

vfdcomm: vfdcomm.o
	gcc vfdcomm.o -o vfdcstate `pkg-config --cflags --libs libmodbus`



clean:
	rm -f vfdcomm.o
	rm -f vfdstate
