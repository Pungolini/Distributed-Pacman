#include "utils.h"
//SERVIDOR



/***************************/
void checkMem(void *ptr)
{
	if(!ptr){
		fprintf(stderr, "Erro a alocar memoria");
		exit(EXIT_FAILURE);
	}
}

void parseArgs(unsigned int *color,int *port, int argc, char const *argv[])
{
	if (argc == 4)
	{
		if ( sscanf(argv[2], " %d", port ) == 1 )
			if (sscanf(argv[3], " %u", color ) == 1)
				if((*color >=0) && (*color <= 10))
					return;
		puts("\n Wrong arguments! Try again!\n");
		puts("Usage: ./client [IP] [PORT] [COLOR] \n");
		puts("Attention: [COLOR] must be an integer on the range [0-10]");
		exit(EXIT_FAILURE);
			
	}else exit(EXIT_FAILURE);
}

//Signal Handler para o SIGINT/SIGPIPE/SIGSTOP

void initSigHandlers(void (*handler)(int))
{
	struct sigaction a;

	sigemptyset(&a.sa_mask);

	a.sa_handler = handler;

	a.sa_flags = 0;
	

	//Definir comportamento quando o processo "apanha" o SIGINT (CTRL+C), serve para terminarmos o servidor/cliente
	if(sigaction( SIGINT, &a, NULL ) == -1){
		fprintf(stderr, "Couldn't define signal handler\n");
		exit(EXIT_FAILURE);
	}	
	
	// Ignorar SIGPIPE, pois todos os send/recv estao protegidos pelo disconnect_client
	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR){
		fprintf(stderr, "Couldn't define signal handler\n");
		exit(EXIT_FAILURE);
	}
	
}



void start_socket(int * sock_fd,const char *address,struct sockaddr_in *_addr, int port, short is_server)
{
  //Verifica a socket
  if ((*sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
	perror("socket: ");
	exit(EXIT_FAILURE);
  }



  _addr->sin_family = AF_INET;
  _addr->sin_port= htons(port);
  _addr->sin_addr.s_addr= INADDR_ANY;

  if (is_server)
  {
   

	// Evitar erro do bind address unavailable
	int enable = 1;
	if (setsockopt(*sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");


	//Faz o bind da socket
	int err = bind(*sock_fd, (struct sockaddr *)_addr, sizeof(*_addr));

	if(err == -1)
	{
	  perror("bind");
	  exit(EXIT_FAILURE);
	}
	puts(" Socket created and binded! \n");
  }else // Se for client inves de bind faz-se connect ao servidor
  {
	if(inet_pton(AF_INET,address, &(_addr->sin_addr)) <= 0)
	{
	  puts("\nInvalid address/ Address not supported \n");
	  exit (-1);
	}

	//Conecta com o server
	if(connect(*sock_fd,(const struct sockaddr *) _addr,sizeof(*_addr)) == -1)
	{
	  puts("Can't connect to the server\n");
	  exit(EXIT_FAILURE);
	}
  }

  puts("Connection succesfully established!\n");
}


void serialize(board_cell *cell)
{
	checkMem(cell);

	cell->color = htonl(cell->color);
	cell->coord.col = htonl(cell->coord.col);
	cell->coord.row = htonl(cell->coord.row);
	cell->element = htonl(cell->element);
	cell->id = htonl(cell->id);
}

void unserialize(board_cell *cell)
{
	checkMem(cell);

	cell->color = ntohl(cell->color);
	cell->coord.col = ntohl(cell->coord.col);
	cell->coord.row = ntohl(cell->coord.row);
	cell->element = ntohl(cell->element);
	cell->id = ntohl(cell->id);
}

/********** ABSTRACAO DO BOARD_CELL ***************/

void set_id(board_cell *ptr,int id)
{
	checkMem(ptr);
	ptr->id = id;
}

void set_clr(board_cell *ptr,int color)
{
	checkMem(ptr);
	ptr->id = color;
}

void set_pos(board_cell *ptr,point *pos)
{
	checkMem(ptr);
	ptr->coord = *pos;
}

void set_char(board_cell *ptr,char elem)
{
	checkMem(ptr);
	ptr->element = elem;
}

void get_id(board_cell *ptr,int *id)
{
	checkMem(ptr);
	*id = ptr->id;
}

void get_clr(board_cell *ptr,int *color)
{
	checkMem(ptr);
	*color = ptr->color;	
}

void get_char_bc(board_cell *ptr,char *ch)
{
	checkMem(ptr);
	*ch = ptr->element;	
}

void get_pos(board_cell *ptr,point *pos)
{
	checkMem(ptr);
	*pos = ptr->coord;
}

void get_coord(board_cell *ptr,int *r,int *c)
{
	*r = ptr->coord.row;
	*c = ptr->coord.col;
}

void init_cell(board_cell *cell, int clr,int id, int elem,int r,int c)
{
	cell->color = clr;
	cell->id = id;
	cell->element = elem;
	cell->coord.row = r;
	cell->coord.col = c;
}

void render_cell(board_cell *to_render)
{
	checkMem(to_render);
	int i,j,clr;
	char ch;

	get_char_bc(to_render,&ch);
	get_coord(to_render,&i,&j);
	get_clr(to_render,&clr);


	SDL_Color color = PALETTE[clr];

	switch(ch)
	{
		case LEMON:
			paint_lemon(j,i);
			break;
		case BRICK:
			paint_brick(j,i);
			break;
		case CHERRY:
			paint_cherry(j,i);
			break;
		case PACMAN:
			paint_pacman(j,i,color.r,color.g,color.b);
			break;
		case MONSTER:
			paint_monster(j,i,color.r,color.g,color.b);
			break;
		case POWERED:
			paint_powerpacman(j,i,color.r,color.g,color.b);
			break;
		case EMPTY:
			clear_place(j,i);
			break;
	}
}