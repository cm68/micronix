TARGET = asz
LIBS = 
CC = gcc
CFLAGS = -g -Wall

all: $(TARGET)

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	rm -f *.o

clobber: clean
	rm -f $(TARGET)

test:	asz
	./asz uhdr.s
	~/src/micronix/src/tools/nm -vrd uhdr.o
	./asz test1.s
	~/src/micronix/src/tools/nm -vrd test1.o
