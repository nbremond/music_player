COMPILER=g++
OPTIONS= -Wall -I/usr/local/include -L/usr/local/lib -lmpdclient -lwiringPi -pthread -lboost_filesystem -lboost_system
DEBUG=yes


ifeq ($(DEBUG),yes)
	OPTIONS+= -g -DDEBUG
endif
COMPILER+= $(OPTIONS)

.PHONY: all clean

all: musicPlayer

musicPlayer: main.o GPIO.o wiring.o
	$(COMPILER) $^ -o $@ 

GPIO.o: GPIO.hpp wiring.o GPIO.cpp
	$(COMPILER) -c GPIO.cpp -o GPIO.o

main.o: GPIO.hpp


%.o: %.cpp %.hpp
	$(COMPILER) -c $< -o $@ 

clean:
	rm *.o musicPlayer
