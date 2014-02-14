BOTWRAPPERSOURCEFILES=botwrapper/main.c
BOTWRAPPERTARGET=keksbot
BOTWRAPPEROBJECTS=$(BOTWRAPPERSOURCEFILES:.c=.o)

LIBKEKSBOTSOURCEFILES=libkeksbot/main.cpp libkeksbot/server.cpp libkeksbot/configs.cpp libkeksbot/eventmanager.cpp libkeksbot/simpleevent.cpp
LIBKEKSBOTTARGET=libkeksbot.so
LIBKEKSBOTOBJECTS=$(LIBKEKSBOTSOURCEFILES:.cpp=.o)

CC=g++
CFLAGS=-c -Wall -fPIC -g
LDFLAGS=-ldl
LDLIBFLAGS=-lircclient -shared
all: $(LIBKEKSBOTSOURCEFILES) $(LIBKEKSBOTTARGET) $(BOTWRAPPERSOURCEFILES) $(BOTWRAPPERTARGET)

rebuild: clean all

$(BOTWRAPPERTARGET): $(BOTWRAPPEROBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(BOTWRAPPEROBJECTS)

$(LIBKEKSBOTTARGET): $(LIBKEKSBOTOBJECTS)
	$(CC) $(LDLIBFLAGS) -o $@ $(LIBKEKSBOTOBJECTS)

.cpp.o:
	$(CC) $(CFLAGS) -o $@ $<

.c.o:
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(LIBKEKSBOTTARGET) $(BOTWRAPPERTARGET) botwrapper/*.o libkeksbot/*.o
