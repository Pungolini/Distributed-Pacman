## Use EXE to represent the name of the program that we're building.
EXE     = server

## SRC is the list of .c files we need to build our program..
SRC     = server.c ../common/utils.c ../common/UI_library.c playerManagement.c com.c board.c 
CC_OPTIONS      = -O2 -Wall
LIBS            = -lm -lSDL2_image -lSDL2 -lpthread

## Define OBJ, which symbolizes all the .o files that the compiler needs
##      to create and then link together in order to create the program.
## Define OBJ, which symbolizes all the .o files that the compiler needs
##      to create and then link together in order to create the program.
bin:
	$(CC) $(SRC)  $(CC_OPTIONS) $(LIBS) -o $(EXE)

#bin1:
#	gcc server.c playerManagement.c board_library.c  -g -Wall  -lpthread  -o server


s:
	./$(EXE)

gdb:
	gdb -ex=r --args ./$(EXE) 

clean:
	rm -f $(EXE) *.o a.out core

hlg:
	valgrind --tool=helgrind --read-var-info=yes ./$(EXE)
gprof:
	gprof $(EXE) gmon.out > gprof_server

depend:         Makefile $(SRC)
	makedepend $(INCLUDES) $(SRC)
