CXX = g++ -DTESTING_=1 -DDEBUG
CC = gcc -DTESTING_=1 -DDEBUG
LDLIBS = -g -lpthread -ldl

SOURCE = $(wildcard *.cpp)
OBJS = $(SOURCE:.c=.o)
LIBS = libsqlite3.a
CFLAGS = -g -Wall

sqltest: ${OBJS} ${LIBS}
	${CXX} ${OBJS} ${LIBS} -o sqltest ${LDLIBS} ${CFLAGS}

libsqlite3.a: sqlite/sqlite3.c
	${CC} -c sqlite/sqlite3.c ${CFLAGS}
	ar rcs libsqlite3.a sqlite3.o


%.o: %.cpp
	${CXX} -c $@ ${LDLIBS} ${CFLAGS}

clean:
	rm -f *.o sqltest

