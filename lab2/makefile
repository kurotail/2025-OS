TARGET 	= my_shell
CC     	= gcc
FLAGS  	= -Wall
OBJ    	= builtin.o command.o shell.o
INCLUDE = ./include/
SRC		= ./src/
BUILD   = ./build

all: $(TARGET) 

$(TARGET): my_shell.c $(OBJ)
	$(CC) $(FLAGS) -o $(BUILD)/$(TARGET) $(addprefix $(BUILD)/, $(OBJ)) $<

%.o: ${SRC}%.c ${INCLUDE}%.h ${BUILD}
	$(CC) -o $(BUILD)/$@ $(FLAGS) -c $<

${BUILD}:
	@mkdir -p $(BUILD)

.PHONY: clean
clean:
	rm -f ${TARGET} *.o out*
clean_obj:
	rm -f *.o
