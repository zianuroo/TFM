C_FLAGS := -g -Wall -Wextra -s
CC := gcc
RM := rm
LINKFLAGS :=

.PHONY: $(TARGET)
.PHONY: clean

VPATH:= ./src/ ./obj/ ./include/

# Path for .c , .h and .o Files 
SRC_PATH := ./src/
OBJ_PATH := ./obj/
INC_PATH := -I ./include

# Executable Name 
TARGET := client
EXEDIR := ../

# Files to compile
OBJ1 := dataBank.o \
		client_funcs.o \
		client_TCP.o \
		client_UDP.o \
		client.o

OBJ := $(patsubst %,$(OBJ_PATH)%,$(OBJ1))

# Build .o first
$(OBJ_PATH)%.o: $(SRC_PATH)%.c
	@echo [CC] $<
	@$(CC) $(C_FLAGS) -o $@ -c $< $(INC_PATH)                  

# Build final Binary
$(EXEDIR)$(TARGET):      $(OBJ)
	@echo [INFO] Creating Binary Executable [$(TARGET)]
	@$(CC) -o $@ $^ $(LINKFLAGS)

# Clean all the object files and the binary
clean:   
	@$(RM) -rfv $(OBJ_PATH)*
	@$(RM) -rfv $(TARGET)
