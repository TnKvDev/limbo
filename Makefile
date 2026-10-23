SRC     = $(wildcard src/*.c)
HDR     = $(wildcard src/*.h)
INI_SRC = lib/iniparser/src/iniparser.c lib/iniparser/src/dictionary.c
CFLAGS  = -Wall -Ilib/iniparser/src
LINK    = -lssl -lcrypto -lpthread
TARGET  = build/limboBot

all: $(TARGET)

$(TARGET): $(SRC) $(HDR) $(INI_SRC)
	@mkdir -p build
	gcc $(CFLAGS) $(SRC) $(INI_SRC) -o $@ $(LINK)

run: $(TARGET)
	./$(TARGET)

.PHONY: all debug run
