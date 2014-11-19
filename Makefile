BOTWRAPPERSOURCEFILES=botwrapper/main.c
BOTWRAPPERTARGET=keksbot
BOTWRAPPEROBJECTS=$(BOTWRAPPERSOURCEFILES:.c=.o)

LIBKEKSBOTSOURCEFILES=libkeksbot/main.cpp libkeksbot/server.cpp libkeksbot/configs.cpp libkeksbot/eventmanager.cpp libkeksbot/simpleevent.cpp libkeksbot/eventinterface.cpp libkeksbot/statichandlers.cpp libkeksbot/stattracker.cpp libkeksbot/stats.cpp libkeksbot/classifiedhandler.cpp libkeksbot/udsserver.cpp libkeksbot/httprelay.cpp libkeksbot/libhandle.cpp libkeksbot/statrequester.cpp libkeksbot/exceptions.cpp libkeksbot/unicode.cpp
LIBKEKSBOTTARGET=libkeksbot.so
LIBKEKSBOTOBJECTS=$(LIBKEKSBOTSOURCEFILES:.cpp=.o)

KEKSCGISOURCEFILES=fcgi/main.cpp
KEKSCGITARGET=keksbot-cgi
KEKSCGIOBJECTS=$(KEKSCGISOURCEFILES:.cpp=.o)

CC=g++
CFLAGS=-c -Wall -fPIC -g -std=c++98
LDFLAGS=-ldl -lsqlite3
LDLIBFLAGS=-lircclient -lcurl -shared
LDCGIFLAGS=-lfcgi
all: $(LIBKEKSBOTSOURCEFILES) $(LIBKEKSBOTTARGET) $(BOTWRAPPERSOURCEFILES) $(BOTWRAPPERTARGET) $(KEKSCGISOURCEFILES) $(KEKSCGITARGET)

rebuild: clean all

$(BOTWRAPPERTARGET): $(BOTWRAPPEROBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(BOTWRAPPEROBJECTS)

$(LIBKEKSBOTTARGET): $(LIBKEKSBOTOBJECTS)
	$(CC) $(LDLIBFLAGS) -o $@ $(LIBKEKSBOTOBJECTS)

$(KEKSCGITARGET): $(KEKSCGIOBJECTS)
	$(CC) $(LDCGIFLAGS) -o $@ $(KEKSCGIOBJECTS)

.cpp.o:
	$(CC) $(CFLAGS) -o $@ $<

.c.o:
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(LIBKEKSBOTTARGET) $(BOTWRAPPERTARGET) $(KEKSCGITARGET) botwrapper/*.o libkeksbot/*.o fcgi/*.o
