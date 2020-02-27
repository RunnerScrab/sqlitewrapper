CXX = g++
CC = gcc
LDLIBS = -g -lpthread -ldl

SOURCE = $(wildcard *.cpp)
OBJS = $(SOURCE:.c=.o)
LIBS = libsqlite3.a


sqltest: ${OBJS} ${LIBS}
	${CXX} ${OBJS} ${LIBS} -o sqltest ${LDLIBS}

libsqlite3.a: sqlite/sqlite3.c
	${CC} -c sqlite/sqlite3.c ${CFLAGS}
	ar rcs libsqlite3.a sqlite3.o


%.o: %.cpp
	${CXX} -c $@ ${LDLIBS}

clean:
	rm -f *.o sqltest

