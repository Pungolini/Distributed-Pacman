#include "client.h"

int _rows,_cols;


int CLIENT_PORT;
volatile int done = 0;

void * sendThread (void * sock)
{
	
	int sock_fd = (intptr_t)sock;
	board_cell send_play;

	SDL_Event event;

	//Iniciar tratamento de SIGINT e SIGPIPE
	initSigHandlers(terminate_client);
	while (!done)
	{
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
					done = SDL_TRUE;
					break;

				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{	
						case SDLK_LEFT:
						case SDLK_UP:
						case SDLK_RIGHT:
						case SDLK_DOWN:
							set_char(&send_play,MONSTER);
							send_play.color = event.key.keysym.sym;
							//set_clr(&send_play,event.key.keysym.sym);
						
							if(write(sock_fd,&send_play,sizeof(send_play)) < 0)
								done = true;
							break;
					}
					//puts("Key down!");
					break;

				case SDL_MOUSEMOTION: // Pacman

					get_board_place(event.button.x,event.button.y,&send_play.coord.col, &send_play.coord.row);
					//printf("Clicked on (%d %d)\n",send_play.coord.row,send_play.coord.col);
					set_char(&send_play,PACMAN);
					if(send(sock_fd,&send_play,sizeof(send_play),0) < 0)
						done = true;

					break;
			}
		}
	}
	close_board_windows();

	// Avisar servidor que acabei
	send_play.element = FINISHED;
	if(send(sock_fd,&send_play,sizeof(send_play),0) < 0)
		exit(EXIT_FAILURE); // De qualquer forma ja estavamos a nos desconectar
	close (sock_fd);
	shutdown(sock_fd,SHUT_RD); 


	return NULL;
}

void* receiveThread (void *arg)
{

	int sock_fd = (intptr_t)arg;
	board_cell to_recv = {0};



	while ((recv(sock_fd, &to_recv, sizeof(to_recv), 0) > 0))
	{
		
		if (to_recv.element == RECEIVE_SCORE)
			printf("PlayerID: %d  | Score:  %d\n\n",(to_recv.id),(to_recv.color));
		else
			render_cell(&to_recv);
	
	}
	close_board_windows();
	close(sock_fd);
	
	return NULL;
}


static void terminate_client(int a)
{
	printf("TERMINATING CLIENT!");
	//fflush(stdout);
	close_board_windows();
	done = 1;
}

int main(int argc, char const *argv[])
{
	struct sockaddr_in server_addr;
	unsigned int player_color;
	int sock_fd;

	//threads
	pthread_t sendThreadId;
	pthread_t receiveThreadId;


	// Verificar argumentos de compilacao
	parseArgs(&player_color,&CLIENT_PORT,argc,argv);


	// Estabelecer ligacao com o servidor
	start_socket(&sock_fd,argv[1],&server_addr,CLIENT_PORT,0);


	// Receber e enviar info necessaria ao servidor,criar o tabuleiro e receber o estado atual do mesmo
	get_initial_info(sock_fd,player_color);
	

	// Criar threads de envio e rececao
	if(pthread_create(&receiveThreadId, NULL, receiveThread, (void*)(intptr_t)sock_fd))
		exit(-1);
	if(pthread_create(&sendThreadId, NULL, sendThread,(void*)(intptr_t)sock_fd))
		exit(-1);


	// Fechar tudo
	pthread_join(sendThreadId, NULL);
	pthread_join(receiveThreadId, NULL);

	return 0;
}

void get_initial_info(int sock_fd, int color)
{

	// Enviar Cor
	if (send(sock_fd,&color,sizeof(color),0) < 0)
		exit(-1);

	// Receber tamanho do tabuleiro
	if(recv(sock_fd, &_rows, sizeof(_rows), 0) < 0)
		exit(-1);
	

	if(recv(sock_fd, &_cols, sizeof(_cols), 0) < 0)
		exit(-1);
		
	// Criar o tabuleiro
	create_board_window(_cols,_rows,color);
}
