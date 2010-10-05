CC = g++
CFLAGS = -g -O3 -DDEBUG -DGIT_REVISION=`git rev-parse HEAD`
SOURCES = MyBot.cc Planet.cc GameState.cc Map.cc Config.cc DoTurn.cc
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

