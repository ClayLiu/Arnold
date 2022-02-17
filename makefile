BIN = ./bin
SRC = ./src
OBJ = ./obj
INC = ./include


SRCS = $(wildcard $(SRC)/*.c)
OBJS = $(patsubst %.c, $(OBJ)/%.o, $(notdir $(SRCS)))

TARGET_NAME = main
TARGET_PATH = $(BIN)/$(TARGET_NAME)

CFLAGS = -std=c11 -I $(INC)

$(TARGET_PATH):$(OBJS)
	gcc $^ -o $@

$(OBJ)/%.o:$(SRC)/%.c
	gcc $(CFLAGS) -c $< -o $@

.PHONY:run
run:$(TARGET_PATH)
	bin\main.exe

.PHONY:clean
clean:
	@echo Clean my ass.
	del /Q /F obj

run_build_arnold_kernel:
	gcc utils\build_arnold_kernel_c.c -o utils\build_arnold_kernel_c.exe
	utils\build_arnold_kernel_c.exe

	