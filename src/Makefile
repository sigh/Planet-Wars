CC = g++
CFLAGS = -g -O3 -Wall -Werror -DDEBUG -DGIT_REVISION=`git rev-parse HEAD`
SOURCES = MyBot.cpp Planet.cpp GameState.cpp Map.cpp Config.cpp DoTurn.cpp
OBJECTS=$(SOURCES:.cpp=.o)
LIBS=-lboost_program_options -lm 

all: MyBot

zip:
	zip -r MyBot *.cpp *.h Makefile strategy

clean:
	rm -rf *.o MyBot *.zip

MyBot: $(OBJECTS)
		$(CC) $(CFLAGS) $(LIBS) $(OBJECTS) -o  MyBot

DoTurn.o: DoTurn.cpp strategy/attack.cpp strategy/defence.cpp strategy/redist.cpp
		$(CC) $(CFLAGS) -c $<

%.o: %.cpp
		$(CC) $(CFLAGS) -c $<

