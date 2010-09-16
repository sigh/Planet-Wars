CC = g++
CFLAGS = -g -O3 -DDEBUG
SOURCES = MyBot.cc Planet.cc PlanetWars.cc Map.cc Config.cc DoTurn.cc
OBJECTS=$(SOURCES:.cc=.o)
INCLUDE=-I/usr/local/boost_1_44_0

all: MyBot

zip:
	zip -r MyBot *.cc *.h Makefile

clean:
	rm -rf *.o MyBot *.zip

MyBot: $(OBJECTS)
		$(CC) $(CFLAGS) $(OBJECTS) -o  MyBot

%.o: %.cc
		$(CC) $(CFLAGS) $(INCLUDE) -c $<

