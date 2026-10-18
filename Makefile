C_SRC = src/*.c src/*.h
LINK =-lssl -lcrypto
TARGET = build/limboBot
LIB = lib/libiniparser.a

all:
	gcc $(C_SRC) -o $(TARGET) $(LINK) $(LIB)

debug:
	gcc $(C_SRC) -o $(TARGET)DBG $(LINK) $(LIB) -g

run:
	./$(TARGET)
