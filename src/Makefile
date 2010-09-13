CC = g++
CFLAGS = -Wall -g -O3
OBJECTS = MyBot.o Planet.o PlanetWars.o Map.o

all: MyBot

zip:
	zip -r MyBot *.cc *.h Makefile

clean:
	rm -rf *.o MyBot *.zip

MyBot: $(OBJECTS)
		$(CC) $(CFLAGS) $(OBJECTS) -o  MyBot

%.o: %.c
		$(CC) $(CFLAGS) -c $<

