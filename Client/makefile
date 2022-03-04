## Use EXE to represent the name of the program that we're building.
EXE     = client

## SRC is the list of .c files we need to build our program..
SRC     = client.c ../common/UI_library.c  ../common/utils.c
CC              = gcc
CC_OPTIONS      = -O2 -Wall 
LIBS            = -lm -lSDL2_image -lSDL2 -lpthread

## Define OBJ, which symbolizes all the .o files that the compiler needs
##      to create and then link together in order to create the program.
## Define OBJ, which symbolizes all the .o files that the compiler needs
##      to create and then link together in order to create the program.
bin:
	$(CC) $(SRC)  $(CC_OPTIONS) $(LIBS) -o $(EXE)

#bin1:
#	gcc client.c playerManagement.c board_library.c  -g -Wall  -lpthread  -o client


c:
	./$(EXE) 127.0.0.1 3002 $(clr)

gdb:
	gdb -ex=r --args ./$(EXE) 127.0.0.1 3002 $(clr)


clean:
	rm -f $(EXE) *.o a.out core

gprof:
	gprof $(EXE) gmon.out > gprof_client

hlg:
	valgrind --tool=helgrind --read-var-info=yes ./$(EXE)
	
depend:         Makefile $(SRC)
	makedepend $(INCLUDES) $(SRC)
