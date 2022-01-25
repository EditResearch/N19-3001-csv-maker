CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c18 -O3 -static
TEST_CFLAGS=-Wall -Wextra -pedantic -std=c18 -Isrc
LIBS=-lsnap7
TARGET=csv_maker
BUILD=build
MODULES=\
main.o

TEST_MODULES=\



all: prepare $(MODULES)
	$(CC) $(CFLAGS) $(MODULES) $(LIBS) -o $(BUILD)/$(TARGET)


main.o: app/main.c
	$(CC) $(CFLAGS) -c app/main.c -o main.o


test: prepare $(TEST_MODULES)
	$(CC) $(TEST_CFLAGS) $(TEST_MODULES) $(LIBS) -o $(BUILD)/autotest
	$(BUILD)/autotest


exec:
	$(BUILD)/$(TARGET)


prepare:
	mkdir -pv $(BUILD)


clean:
	rm -rfv $(BUILD)
	rm -vf *.o
