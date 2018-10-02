BOTWRAPPERSOURCEFILES=botwrapper/main.c
BOTWRAPPERTARGET=keksbot
BOTWRAPPEROBJECTS=$(BOTWRAPPERSOURCEFILES:.c=.o)

LIBKEKSBOTSOURCEFILES=libkeksbot/main.cpp libkeksbot/server.cpp libkeksbot/configs.cpp libkeksbot/eventmanager.cpp libkeksbot/simpleevent.cpp libkeksbot/eventinterface.cpp libkeksbot/statichandlers.cpp libkeksbot/stattracker.cpp libkeksbot/stats.cpp libkeksbot/classifiedhandler.cpp libkeksbot/udsserver.cpp libkeksbot/httprelay.cpp libkeksbot/libhandle.cpp libkeksbot/statrequester.cpp libkeksbot/exceptions.cpp libkeksbot/unicode.cpp libkeksbot/mensa.cpp libkeksbot/quizzer.cpp
LIBKEKSBOTTARGET=libkeksbot.so
LIBKEKSBOTOBJECTS=$(LIBKEKSBOTSOURCEFILES:.cpp=.o)

KEKSCGISOURCEFILES=fcgi/main.cpp
KEKSCGITARGET=keksbot-cgi
KEKSCGIOBJECTS=$(KEKSCGISOURCEFILES:.cpp=.o)

CXX=c++
CC=cc
CXXFLAGS=-c -I./external -Wall -fPIC -g -std=c++17
CFLAGS=-c
LDFLAGS=-ldl -lsqlite3
LDLIBFLAGS=-lircclient -lcurl -shared
LDCGIFLAGS=-lfcgi

all: $(LIBKEKSBOTSOURCEFILES) $(LIBKEKSBOTTARGET) $(BOTWRAPPERSOURCEFILES) $(BOTWRAPPERTARGET) $(KEKSCGISOURCEFILES) $(KEKSCGITARGET)

rebuild: clean all

install: all
	install -m 0755 -d $(DESTDIR)/keksbot
	install -m 0755 $(BOTWRAPPERTARGET) $(DESTDIR)/keksbot
	install -m 0755 $(KEKSCGITARGET) $(DESTDIR)/keksbot
	install -m 0644 $(LIBKEKSBOTTARGET) $(DESTDIR)/keksbot
	install -m 0644 *.txt $(DESTDIR)/keksbot

$(BOTWRAPPERTARGET): $(BOTWRAPPEROBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(BOTWRAPPEROBJECTS)

$(LIBKEKSBOTTARGET): $(LIBKEKSBOTOBJECTS)
	$(CXX) $(LDLIBFLAGS) -o $@ $(LIBKEKSBOTOBJECTS)

$(KEKSCGITARGET): $(KEKSCGIOBJECTS)
	$(CXX) $(LDCGIFLAGS) -o $@ $(KEKSCGIOBJECTS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ $<

.c.o:
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(LIBKEKSBOTTARGET) $(BOTWRAPPERTARGET) $(KEKSCGITARGET) botwrapper/*.o libkeksbot/*.o fcgi/*.o

.PHONY: install all rebuild clean

