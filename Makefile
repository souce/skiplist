CC=gcc
BIN = test
BIN_PATH = ./
EXAMPLE_PATH = ./example

SRC_PATH = ./
SRC_FILE = $(wildcard $(SRC_PATH)/*.c)
SRC_OBJS = $(patsubst %.c,%.o,$(SRC_FILE))

INC_PATH = -I$(SRC_PATH)
CFLAGS = -g -O0 -Wall $(INC_PATH)

clean:
	$(RM) $(SRC_PATH)/*.o $(BIN_PATH)/map $(BIN_PATH)/array

array: $(SRC_OBJS) 
	$(CC) -o $(BIN_PATH)/$@ $(SRC_OBJS) $(EXAMPLE_PATH)/array.c $(EXAMPLE_PATH)/utils.c $(CFLAGS)
	@echo "compile '$@' success!";

map: $(SRC_OBJS) 
	$(CC) -o $(BIN_PATH)/$@ $(SRC_OBJS) $(EXAMPLE_PATH)/map.c $(EXAMPLE_PATH)/utils.c $(CFLAGS)
	@echo "compile '$@' success!";

