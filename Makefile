program_NAME := irc
CC=gcc

OUTPUT_FOLDER = output

OUTPUT_OPTION = -o $(OUTPUT_FOLDER)/$@



program_C_SRCS := $(wildcard *.c)
#program_OBJS := ${program_C_SRCS:.c=.o}
#program_OBJS_OUTPUT := $(wildcard output/*.o)

all: $(program_NAME)

$(program_NAME): $(program_C_SRCS)
	$(CC) $^ $(OUTPUT_OPTION)
	./$(OUTPUT_FOLDER)/$(program_NAME)