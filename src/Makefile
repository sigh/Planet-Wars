CC=g++

all: MyBot

zip:
	zip -r MyBot *.cc *.h Makefile

clean:
	rm -rf *.o MyBot *.zip

MyBot: MyBot.o Planet.o PlanetWars.o Map.o
