TARGET = $(BIN_DIR)/webServer.out
#Add new object
OBJECT = $(OBJ_DIR)/web-server.o


# flags de gcc
CFLAGS = -Wall -I$(INC_DIR)

INC_DIR = ./inc
OBJ_DIR = ./obj
SRC_DIR = ./src
BIN_DIR = ./bin

$(TARGET) : $(OBJECT)
	mkdir -p $(BIN_DIR)
	gcc $(OBJECT) -o $(TARGET)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	mkdir -p $(OBJ_DIR)
	gcc -c -MD $(CFLAGS) $< -o $@

#Metodo Tom Tromey
-include $(OBJ_DIR)/*.d

.PHONY: clean
clean :
	@rm -r $(OBJ_DIR) $(BIN_DIR)

.PHONY: run
run : 
	@$(BIN_DIR)/webServer.out

.PHONY: runArgs 
runArgs : 
	@$(BIN_DIR)/webServer.out $(ARGS)