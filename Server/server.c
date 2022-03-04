#include "server.h"


BOARD game_board;

int server_sock_fd;
pthread_t main_thread; 


void* receivePlays(void *_id)
{
	char elem;
	int id =(intptr_t)_id;
	playerStruct player = *(get_player_ptr(player_arr,id));
	int sock_fd = player.player_fd;
	board_cell play_move,old_play = {.id = id};

	//struct pollfd fds = {.fd = sock_fd, .events = POLLIN};
	pthread_t score_tid,poll_m_tid,poll_p_tid;


	timer_ poll_pac,poll_monster;
	poll_pac.player_info = poll_monster.player_info = &player;


	// Criar thread para ver se o jogador esta inativo durante 30s
	if(pthread_create(&poll_p_tid, NULL,poll_pacman_inactivity, (void*)&poll_pac)){
		fprintf(stderr, "Couldn't create thread\n");
		exit(EXIT_FAILURE);
	}


	if(pthread_create(&poll_m_tid, NULL,poll_monster_inactivity, (void*)&poll_monster)){
		fprintf(stderr, "Couldn't create thread\n");
		exit(EXIT_FAILURE);
	}

	if(pthread_create(&score_tid, NULL, send_score_thread,(void*)(intptr_t)sock_fd )){
		fprintf(stderr, "Couldn't create thread\n");
		exit(EXIT_FAILURE);
	}


	set_field(TIME_M,id,RST);
	set_field(TIME_P,id,RST);

	// Enviar tabuleiro, pacman e monstro ao novo jogador
	send_initial_data(id,sock_fd);

	// Loop principal para rececao de jogadas
	while (1)
	{
		if (done_signal) // Fechar o socket do servidor
		{
			close(server_sock_fd);
			shutdown(server_sock_fd,SHUT_RDWR);
			break;
		}

		else if (_nplayers(RET) == 1)
			set_field(SET_SCORE,id,RST); // so ha 1 jogador conectado, reiniciar a pontuacao
		
		
		if( recv(sock_fd,&play_move,sizeof(play_move),0) > 0)
		{
			get_char_bc(&play_move,&elem);
			
			if (elem != FINISHED)
			{	
				// Atualizar hora de rececao do boneco
				if (elem == PACMAN)
					set_field(TIME_P,id,RST);
				else
					set_field(TIME_M,id,RST);

				// Obter coordenadas da posicao "atual" do pac/monstro (que ainda esta no tabuleiro)	
				get_curr_pos(id,&old_play,&play_move);
				
				
				// Processar a jogada
				process_play(id,&old_play,&play_move);


				//printMat(game_board,rows,cols);
			}
			else 
			{
				// Desconectar o cliente e limpar as personagens do tabuleiro
				disconnect_client(id,sock_fd);
				break;
			}
		}else break;	

	}
	printMat(game_board,rows,cols);

	reset_all();

	// Destruir as threads de tempo

	pthread_cancel(poll_p_tid);
	pthread_join(poll_p_tid,NULL);

	pthread_cancel(poll_m_tid);
	pthread_join(poll_m_tid,NULL);

	pthread_cancel(score_tid);
	pthread_join(score_tid,NULL);


	pthread_exit(NULL);
	
	return NULL;
}

static void handleSigInt(int a)
{ 
	done_signal = 1;
}

int main (int argc, char * argv[])
{


	srand(time(NULL));

	struct sockaddr_in local_addr;
	int player_color,playerCounter = 0;
	int client_fd = -1,player_id;
	//Vetor de threads
	pthread_t timer_tid,thread_id[MAXPLAYERS];
	int kill_timer_thread = 0;


	initSigHandlers(handleSigInt);

	// Inicializar os mutexes de uso geral
	init_sync();


	//Cria a socket no SERVER_PORT
	start_socket(&server_sock_fd,NULL,&local_addr, SERVER_PORT,1);

	// Listen for incoming connections
	if(listen(server_sock_fd, MAXPLAYERS) == -1)
		exit(EXIT_FAILURE);


	player_arr = init_array(MAXPLAYERS);


	// Criar o tabuleiro
	game_board = init_board("board.txt");
	count_elements(game_board);

	
	//Criar thread para o timer de 2s das frutas
	if(pthread_create(&timer_tid, NULL, fruit_spawn_thread, (void*)(intptr_t)kill_timer_thread))
		exit(EXIT_FAILURE);

	create_board_window(cols,rows,IS_SERVER);


	//Fica a espera dos clientes
	puts("Waiting for players...\n");
	while(!done_signal)
	{

		if((client_fd = accept(server_sock_fd, NULL, NULL)) == -1)
			break;
		//Aceita um cliente, mas so se houver espaco no tabuleiro
		if((get_empty_cells() < 4) || (playerCounter >= MAXPLAYERS))
		{
			puts("No space available on the board, try later!");
			close(client_fd);
			continue;
		}

		printf("\n\nNew client connected (%d)!\n", playerCounter + 1);

		
		//Incrementa o contador de jogadores
		rwLock(W_LOCK,&nplayers_lock);
		NUMPLAYERS = ++playerCounter;
		rwLock(UNLOCK,&nplayers_lock);


		// Receber a cor associada ao jogador
		if(recv(client_fd,&player_color,sizeof(player_color),0) <= 0) 
			exit(EXIT_FAILURE);


		// Associar um numero (ID) ao jogador 
		player_id = playerCounter - 1;

		//Adiciona o novo jogador no vetor
		player_arr = insert_player(player_arr,player_id,client_fd,player_color);


		//Cria a thread para este jogador
		if(pthread_create(&thread_id[player_id], NULL, receivePlays, (void*)(intptr_t)player_id))
			exit(EXIT_FAILURE);

		if(pthread_detach(thread_id[player_id]))
			exit(EXIT_FAILURE);


	}
	puts("\nServer terminated!\n");

	puts("Cleaning ressources...");

	// Assim que a thread der return, destruir
	kill_timer_thread = JOIN_THREAD;
	pthread_cancel(timer_tid);
	pthread_join(timer_tid,NULL);
	
	free_board(game_board,cols);
	free_array(player_arr);
	
	
	mutex(DESTROY,&global_mutex);
	mutex(DESTROY,&pacman_mutex);
	mutex(DESTROY,&monster_mutex);
	rwLock(DESTROY,&globals_lock);
	rwLock(DESTROY,&nplayers_lock);


	close(server_sock_fd);
	close_board_windows();

	puts("Ressources freed, exiting...");
	SDL_Delay(500);

	return EXIT_SUCCESS;
}



