CC = g++
CFLAGS = -g -O3 -DDEBUG
SOURCES = MyBot.cc Planet.cc PlanetWars.cc Map.cc Config.cc DoTurn.cc
OBJECTS=$(SOURCES:.cc=.o)
LIBS=-lboost_program_options -lm 

all: MyBot

zip:
	zip -r MyBot *.cc *.h Makefile

clean:
	rm -rf *.o MyBot *.zip

MyBot: $(OBJECTS)
		$(CC) $(CFLAGS) $(LIBS) $(OBJECTS) -o  MyBot

%.o: %.cc
		$(CC) $(CFLAGS) -c $<

