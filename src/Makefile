CC = g++
CFLAGS = -g -O3 -Wall -Werror -DDEBUG -DGIT_REVISION=`git rev-parse HEAD`
SOURCES = MyBot.cc Planet.cc GameState.cc Map.cc Config.cc DoTurn.cc strategy/attack.cc strategy/defence.cc strategy/redist.cc strategy/flee.cc
OBJECTS=$(SOURCES:.cc=.o)
LIBS=-lboost_program_options -lm 

all: MyBot

zip:
	zip -r MyBot *.cc *.h Makefile strategy

clean:
	rm -rf *.o MyBot *.zip strategy/*.o

MyBot: $(OBJECTS)
		$(CC) $(CFLAGS) $(LIBS) $(OBJECTS) -o  MyBot

%.o: %.cc
		$(CC) $(CFLAGS) -c $< -o $@

