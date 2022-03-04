
#include "com.h"


volatile int done_signal = 0;


int send_data(int id,int sock_fd, void* _buff, size_t _n)
{
	int ret = -1;

	if((ret = send(sock_fd, _buff, _n,0)) < 0)
	{
		puts("Unable to send data to socket");
		disconnect_client(id,sock_fd);
	}
	return ret;
}



int send_board(BOARD board,array arr,int id,int sock_fd)
{
	board_cell to_send = {0};
	int ret = 0;

	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			if(get_cell_type(board,i,j) != EMPTY)
			{
				to_send = *get_cell_ptr(board,i,j);
				render_cell(&to_send);
				if((ret = send_data(id,sock_fd,&to_send,sizeof(to_send))) < 0)
					return ret;

			}
		}
	}
	return ret;
	
}


void send_cell_to_all(BOARD board,array arr, board_cell *cell)
{
	int fd;
	playerStruct *player;
	board_cell to_send = {0};
	int r,c;
	get_coord(cell,&r,&c);

	// Cell tem todos campos completos, meter no vetor
 	update_cell(board,cell->id,cell);

	for(int i = 0; i < MAXPLAYERS; i++)
	{	
		player = get_player_ptr(arr,i);

		if(get_player_id(player) == UNAVAILABLE) continue;
		
		fd = get_player_fd(player);

		rwLock(R_LOCK,&board[r][c].rwlock);
		to_send = board[r][c];
		rwLock(UNLOCK,&board[r][c].rwlock);

		// Enviar a estrutura e atualizar no monitor do servidor
		render_cell(&to_send);
		send_data(i,fd,&to_send,sizeof(to_send));
	}
}

void update_pos(board_cell *old_play,board_cell *play)
{
	board_cell tmp = *old_play;

	// Limpar a celula antiga
	set_id(&tmp,UNAVAILABLE);
	set_char(&tmp,EMPTY);
	clear_cell(tmp.coord.row,tmp.coord.col);

	send_cell_to_all(game_board,player_arr,&tmp);

	// Enviar nova jogada
	if (play)
		send_cell_to_all(game_board,player_arr,play); 

}


void send_initial_data(int id, int sock_fd)
{
	board_cell monster,pacman;
	short num_clients;

	//Enviar numero de linhas do tabuleiro
	send_data(id,sock_fd, &rows, sizeof(rows));

	//Enviar numero de colunas do tabuleiro
	send_data(id,sock_fd, &cols, sizeof(cols));


	// Enviar tabuleiro ao jogador e novas pecas aos outros
	if(send_board(game_board,player_arr,id,sock_fd) > 0)
	{
		// Gerar posicao para o monstro e pacman do jogador
		generate_coords(id,&monster, &pacman);

		//Enviar pacman e monstro
		send_cell_to_all(game_board,player_arr,&monster);
		send_cell_to_all(game_board,player_arr,&pacman);

		
		if((num_clients = connected()) > 1)
		{
			board_cell fruit;
			//Enviar frutas
			spawn_fruit(&game_board,num_clients,&fruit );
			send_cell_to_all(game_board,player_arr,&fruit);

			spawn_fruit(&game_board,num_clients,&fruit );
			send_cell_to_all(game_board,player_arr,&fruit);
		}
	}
}



void remove_characters(int player_id)
{
	board_cell pac = {0},ghost= {0},fruit1= {0},fruit2= {0};

	// Obter posicao do pac, monstro e 2 frutas a apagar do tabuleiro
	get_char_pos(&pac,player_id,POWERED);
	get_char_pos(&pac,player_id,PACMAN);
	get_char_pos(&ghost,player_id,MONSTER);


	rwLock(W_LOCK,&player_arr[player_id].rwlock);
	player_arr[player_id].playerID = UNAVAILABLE; // Cliente ja saiu, nao enviar para ele
	rwLock(UNLOCK,&player_arr[player_id].rwlock);

	// Limpar as celulas e enviar
	int r,c;

	get_coord(&pac,&r,&c);
	clear_cell(r,c);
	send_cell_to_all(game_board,player_arr,&game_board[r][c]);


	get_coord(&ghost,&r,&c);
	clear_cell(r,c);
	send_cell_to_all(game_board,player_arr,&game_board[r][c]);

	// Apagar 2 frutas

	get_char_pos(&fruit1,-1,LEMON);
	get_coord(&fruit1,&r,&c);
	clear_cell(r,c);
	send_cell_to_all(game_board,player_arr,&game_board[r][c]);


	get_char_pos(&fruit2,-1,LEMON);
	get_coord(&fruit2,&r,&c);
	clear_cell(r,c);
	send_cell_to_all(game_board,player_arr,&game_board[r][c]);
}

void disconnect_client(int id,int sock_fd)
{
	// Remover o pacman e monstro do jogador do tabuleiro
	remove_characters(id);

	// Fechar a socket
	close(sock_fd);

	// Atualizar o numero de jogadores atualmente conectados
	_nplayers(DEC);
}

void *poll_monster_inactivity(void * poll_time)
{
	mutex(LOCK,&monster_mutex);
	timer_ *time_s = (timer_ *)poll_time;
	mutex(UNLOCK,&monster_mutex);

	int fd = get_player_fd(time_s->player_info),
		id = get_player_id(time_s->player_info),speed_m;

	clock_t dFirst=0,dt,first = 0;//,second,dSecond;
	board_cell buff;

	while(!done_signal)
	{
		/********** REGRA DA VELOCIDADE (2cells/s) *********************/	
		speed_m = get_field(SET_SPEED_M,id);
		if(speed_m == 1)
			first = clock();// Tempo de chegada da 1a jogada
			
		else if(speed_m >= 2)
		{			
			dFirst = time_ms((clock() - first));
			
			while (1)
			{
				dFirst = time_ms((clock() - first));

				if (dFirst >= 1000)
				{
					// Reinicializar o speed counter
					set_field(SET_SPEED_M,id,RST);
					break;
				}
				//printf("First: %ld ms\tdFirst: %ld ms\n",first,dFirst);

				// Descartar as jogadas recebidas nesse periodo
				//board_cell old;
				if (recv(fd,&buff,sizeof(buff),0) > 0)
				{
					if(buff.element == PACMAN)
					{
						//get_curr_pos(id,&old,&buff);
						//process_play(id,&old,&buff);
					}
				}	
				else disconnect_client(id,fd);
			}

		}

		/********** REGRA DA INATIVIDADE (30s) *********************/


		// Calculo do tempo entre a ultima jogada do monstro
		dt = time_ms((clock() - get_char_time(id,MONSTER)));

		// Se passaram mais de 30s, mudar para uma posicao aleatoria
		if(dt > WAIT_30_SECONDS)
		{
			board_cell old_pos,new_pos;
			init_cell(&new_pos,get_field(GET_CLR,id),id,MONSTER,0,0);


			// Gerar nova posicao para o jogador
			generate_pos(&new_pos.coord);

			get_curr_pos(id,&old_pos,&new_pos);
			
			// Enviar a todos a nova posicao
			update_pos(&old_pos,&new_pos);

			// Reinicializar o timer
			set_field(TIME_M,id,RST);
		}

	}

	return NULL;
}

void *poll_pacman_inactivity(void * poll_time)
{
	mutex(LOCK,&pacman_mutex);
	timer_ *time_s = (timer_ *)poll_time;
	mutex(UNLOCK,&pacman_mutex);

	int fd = get_player_fd(time_s->player_info),
		id = get_player_id(time_s->player_info),speed_p;
	

	clock_t dFirst=0,dt,first = 0;
	board_cell buff;
	
	while(!done_signal)
	{	
		/********** REGRA DA VELOCIDADE (2cells/s) *********************/	
		speed_p = get_field(SET_SPEED_P,id);
		if(speed_p == 1)
			first = clock();// Tempo de chegada da 1a jogada
			
		else if(speed_p >= 2)
		{
			//sem_post(&time_s->sem);
			dFirst = time_ms((clock() - first));
			while (1)
			{
				//printf("First: %ld ms\tdFirst: %ld ms\n",time_ms(first),dFirst);
				dFirst = time_ms((clock() - first));
				if (dFirst >= 1000)
				{
					// Reinicializar o speed counter
					set_field(SET_SPEED_P,id,RST);
					break;
				}
				// Descartar as jogadas de pacman recebidas nesse periodo
				if (recv(fd,&buff,sizeof(buff),0) > 0)
				{
					//sem_wait(&time_s->sem);
					if(buff.element == MONSTER)
					{
						//board_cell old
						//get_curr_pos(id,&old,&buff);
						//process_play(id,&old,&buff);
					}
				}	
				else disconnect_client(id,fd);
			}
		}

		/********** REGRA DA INATIVIDADE (30s) *********************/	

		// Calculo do tempo entre a ultima jogada do monstro
		dt = time_ms((clock() - get_char_time(id,PACMAN)));

		if(dt >= WAIT_30_SECONDS)
		{
			board_cell old_pos,new_pos;
			init_cell(&new_pos,get_field(GET_CLR,id),id,PACMAN,0,0);

			// Gerar nova posicao para o jogador
			generate_pos(&new_pos.coord);

			// Obter a posicao a apagar do tabuleiro
			get_curr_pos(id,&old_pos,&new_pos);
			
			// Enviar a todos
			update_pos(&old_pos,&new_pos); // Atualizar

			// Reinicializar o timer
			set_field(TIME_P,id,RST);
		}
	}
	return NULL;
}

/*****************************************************************
Funcao que faz aparecer uma nova fruta no tabuleiro, depois de 
2s. A thread precisa ser acordada, e tal acontece depois de algum
boneco comer uma fruta do tabuleiro
*******************************************************************/

void * fruit_spawn_thread(void *_flag)
{

	int NP,flag = (intptr_t)_flag;
	board_cell fruit_pos = {0};

	while (!done_signal)
	{
		sem_wait(&fruit_sem);
		
		if (flag == JOIN_THREAD)
			break;

		else
		{
			sleep(2);
			NP = _nplayers(RET);
			spawn_fruit(&game_board,NP,&fruit_pos);
			send_cell_to_all(game_board,player_arr,&fruit_pos);

		}
	}
	return NULL;	
}


/*************************************************************
Funcao que envia a pontuacao de todos os clientes conectados
a todos os clientes conectados
*************************************************************/
void send_scoreboard()
{
	// O .element desta estrutura vai ser um sinalizador para o cliente saber que vai receber o score e nao uma jogada
	// O .color vai ser a pontuacao do .id
	board_cell score_info = {.element = RECEIVE_SCORE};
	int id,fd,scr;

	for(int i = 0;i < MAXPLAYERS; i++)
	{
		rwLock(R_LOCK,&player_arr[i].rwlock);
		id = player_arr[i].playerID;
		rwLock(UNLOCK,&player_arr[i].rwlock);

		if (id != UNAVAILABLE)
		{
			

			rwLock(R_LOCK,&player_arr[i].rwlock);
			scr = player_arr[i].score;
			rwLock(UNLOCK,&player_arr[i].rwlock);

			score_info.id = (id);
			score_info.color = (scr);

			// Enviar o score do jogador id a todos os outros
			for(int i = 0;i < MAXPLAYERS; i++)
			{
				rwLock(R_LOCK,&player_arr[i].rwlock);
				
				id = player_arr[i].playerID;
				fd = player_arr[i].player_fd;
				rwLock(UNLOCK,&player_arr[i].rwlock);

				if (id == UNAVAILABLE) continue;

				send_data(id,fd,&score_info,sizeof(score_info));
			}
			
		}
		
	}
}

void *send_score_thread(void *_fd)
{
	while (!done_signal)
	{
		sleep(WAIT_60_SECONDS);
		send_scoreboard();
	}
	pthread_exit(NULL);
	return NULL;
}
/*************************************************************** 
Funcao que trata das interacoes entre bonecos diferentes de 
jogadores diferentes
Trata tambem da transformacao SP->P apos comer 2 monstros


flag = 0 -> interacao M/P ou SuperPacman/M
flag = 1 -> interacao P/M ou M/ SuperPacman
***************************************************************/

void eat_character(board_cell *old_play,board_cell *play,int flag)
{
	int eaten_id=-1,eater_id=-1,r,c,x,y;
	board_cell eaten,eater;
	get_coord(play,&x,&y);
	get_coord(old_play,&r,&c);
	
	if (!flag)
	{
		eaten = *get_cell_ptr(game_board,x,y);
		eater = *get_cell_ptr(game_board,r,c);

		get_id(&eaten,&eaten_id);
		get_id(&eater,&eater_id);

		// Fazer desaparecer o boneco comido
		SWAP(eaten.coord,eater.coord);
		update_pos(&eaten,&eater);
	}
	else if(flag == 1)
	{
		eaten = *get_cell_ptr(game_board,r,c);
		eater = *get_cell_ptr(game_board,x,y);

		get_id(&eaten,&eaten_id);
		get_id(&eater,&eater_id);

		// Fazer desaparecer o boneco comido
		update_pos(&eaten,NULL);
	}

	// Atualizar a pontuacao do jogador com o monstro/superpacman
	set_field(SET_SCORE,eater_id,INC);

	// gerar nova posicao para character comido
	generate_pos(&eaten.coord);
		
	send_cell_to_all(game_board,player_arr,&eaten);

	// Atualizar quantidade de monstros comidos e transformar o superpacman em normal, caso tenha comido 2 monstros
	if(eater.element == POWERED)
	{
		set_field(SET_EAT,eater_id,INC);

		if( get_field(SET_EAT,eater_id) == 2)
		{
			set_field(SET_EAT,eater_id,RST); // Monstros comidos = 0
			
			set_char(&eater,PACMAN); /// Transformar SP -> P

			// Apagar character comido e render SP em P	
			send_cell_to_all(game_board,player_arr,&eater);
			
		}
	}
}

int process_play(int id,board_cell * old_play,board_cell *play)
{
	int o_x,o_y,p_x,p_y,dir = 0,next_move = INVALID;
	set_id(play,id);

	if (is_monster(old_play))
		get_clr(play,&dir);

	count_elements(game_board);

	if ((next_move = play_is_valid(game_board,id,old_play,play)) != INVALID)
	{
		play->color = get_field(GET_CLR,id); 
		get_coord(old_play,&o_x,&o_y);
		get_coord(play,&p_x,&p_y);


		// Se nao se mudou de posicao,nao processar. evita-se enviar a mesma posicao para todos os jogadores
		if(same_cells(old_play,play))
			return SAME_PLAY;
		else
		{
			if(is_monster(play)) // Para a regra da velocidade
			{
				if (get_field(SET_SPEED_M,id) < 2)
					set_field(SET_SPEED_M,id,INC);
			}
			else
			{
				if (get_field(SET_SPEED_P,id) < 2)
					set_field(SET_SPEED_P,id,INC);
			}
		}
		

		if (next_move == STEP) // Avancar numa celula vazia
			update_pos(old_play,play); // Atualizar

		else if (next_move == BOUNCE)
		{
			int ret = bounce(old_play,play,dir);
			if(ret > 0)
			{
				update_pos(old_play,play); // Atualizar
				if(ret == 1)
					set_field(SET_SCORE,id,INC); // Fez-se bounce para cima de uma fruta
			}

		}
		else if (next_move == EAT_FRUIT)
		{
			// Comer a fruta e transformar o pacman e superpacman
			if (is_pacman(old_play))
				set_char(play,POWERED);
			
			update_pos(old_play,play);
			
			// Aumentar a pontuacao
			set_field(SET_SCORE,id,INC);

			// Acordar thread para fazer spawn de uma nova fruta
			sem_post(&fruit_sem);
			return WAIT2;
		}
		else if (next_move == EAT_CHAR)
		{
			char old_elem = get_cell_type(game_board,o_x,o_y);
			char new_elem = get_cell_type(game_board,p_x,p_y);

			//Same player characters interaction
			if (cell_is_mine(game_board,id,play->coord.row,play->coord.col))
			{
				set_char(play,new_elem);

				swap_positions(old_play,play);
				send_cell_to_all(game_board,player_arr,old_play);
				send_cell_to_all(game_board,player_arr,play);
				return 0;
			}
			//Same type, different players, characters interaction
			else if(chars_are_equal(old_elem,new_elem)) // Se for pacman/pacman ou Monstro/monstro trocar de posicao
			{
				play->color = get_cell_clr(game_board,p_x,p_y);
				play->id = get_cell_id(game_board,p_x,p_y);
				puts("SWAP P/P");
				swap_positions(old_play,play);
				set_char(old_play,new_elem);
				send_cell_to_all(game_board,player_arr,old_play);
				send_cell_to_all(game_board,player_arr,play);

				return 0;
			}
			else
			{
				// Interacao Monstro -> Pacman 
				if( ((old_elem == MONSTER) && (new_elem == PACMAN)) )
					eat_character(old_play,play,0);
					
				// Interacao Pacman -> Monstro
				else if((old_elem == PACMAN) && (new_elem == MONSTER))
					eat_character(old_play,play,1);

				
				if( (old_elem == POWERED) || (new_elem == POWERED) )
				{
					// Interacao Monstro -> SuperPacman
					if( ((old_elem == MONSTER) && (new_elem == POWERED)) )
						eat_character(old_play,play,1);
						
						
					// Interacao SuperPacman -> Monstro
					else if((old_elem == POWERED) && (new_elem == MONSTER))
						eat_character(old_play,play,0);
				}				
			}
		}
	}else return INVALID;

	return 0;
}