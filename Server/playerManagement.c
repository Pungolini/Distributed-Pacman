#include "playerManagement.h"


array player_arr;


array init_array(int n_players)
{
	mutex(LOCK,&global_mutex);
	array arr = calloc(n_players,sizeof(*arr));
	checkMem(arr);

	for(int i = 0; i < n_players; i++)
	{
		arr[i].playerID = UNAVAILABLE;
		if(pthread_rwlock_init(&arr[i].rwlock,NULL))
			exit(-1);	
	}
	
	mutex(UNLOCK,&global_mutex);
	return arr;
}

void free_array(array arr)
{
	mutex(LOCK,&global_mutex);
	for(int i = 0; i < MAXPLAYERS; i++)
		rwLock(DESTROY,&arr[i].rwlock);
	

	free(arr);
	mutex(UNLOCK,&global_mutex);
}


/************************************************
Inicializa a estrutura de um novo jogador no vetor
player_fd: return do accept deste novo jogador
*************************************************/
array insert_player(array arr ,int player_id,int player_fd, int player_color)
{
	int index = player_id;

	rwLock(W_LOCK,&arr[index].rwlock);
	set_player_id(&arr[index],player_id);
	set_player_fd(&arr[index],player_fd);
	set_player_color(&arr[index],player_color);

	rwLock(UNLOCK,&arr[index].rwlock);

	// Score e monster_eaten ja estao a 0 porque vetor foi inicializado com calloc
	return arr;
}



playerStruct *get_player_ptr(array arr, int id)
{

	rwLock(R_LOCK,&arr[id].rwlock);
	playerStruct *ptr = &arr[id];
	rwLock(UNLOCK,&arr[id].rwlock);
	return ptr;
}

int get_vect_color(array arr,int id)
{
	rwLock(R_LOCK,&arr[id].rwlock);
	int color = arr[id].color;
	rwLock(UNLOCK,&arr[id].rwlock);

	return color;
}
/*************************************************************
Aplica op player_arr[id].fied
op = RST = 0 -> poe o campo a 0
op = INC = 1 -> Incrementa o valor do campo
op = DEC = 2 -> Derementa o valor do campo
*************************************************************/
void set_field(char field,int id,int op)
{
	
	switch (field)
	{
		case SET_SCORE:

			rwLock(W_LOCK,&player_arr[id].rwlock);
			if(op == RST)
				player_arr[id].score = 0;
			else if (op == INC)
				player_arr[id].score++;
			else if(op == DEC)
				player_arr[id].score--;
			rwLock(UNLOCK,&player_arr[id].rwlock);
			break;

		case SET_EAT:

			rwLock(W_LOCK,&player_arr[id].rwlock);
			if(op == RST)
				player_arr[id].monster_eaten = 0;
			else if (op == INC)
				player_arr[id].monster_eaten++;
			else if(op == DEC)
				player_arr[id].monster_eaten--;
			rwLock(UNLOCK,&player_arr[id].rwlock);
			break;

		case SET_SPEED_P:

			rwLock(W_LOCK,&player_arr[id].rwlock);
			if(op == RST)
				player_arr[id].speed_pac = 0;
			else if (op == INC)
				player_arr[id].speed_pac++;
			else if(op == DEC)
				player_arr[id].speed_pac--;
			rwLock(UNLOCK,&player_arr[id].rwlock);
			break;

		case SET_SPEED_M:

			rwLock(W_LOCK,&player_arr[id].rwlock);
			if(op == RST)
				player_arr[id].speed_monster = 0;
			else if (op == INC)
				player_arr[id].speed_monster++;
			else if(op == DEC)
				player_arr[id].speed_monster--;
			rwLock(UNLOCK,&player_arr[id].rwlock);
			break;

		case TIME_M:


			if(op == RST)
			{
				rwLock(W_LOCK,&player_arr[id].rwlock);
				player_arr[id].monster_time = clock();
				rwLock(UNLOCK,&player_arr[id].rwlock);
			}
			break;

		case TIME_P:

			if(op == RST)
			{
				rwLock(W_LOCK,&player_arr[id].rwlock);
				player_arr[id].pacman_time = clock();
				rwLock(UNLOCK,&player_arr[id].rwlock);
			}
			break;		
	}
}

clock_t get_char_time(int id, char ch)
{
	clock_t ret;
	rwLock(R_LOCK,&player_arr[id].rwlock);
	if(ch == PACMAN)
		ret = player_arr[id].pacman_time;
	else ret = player_arr[id].monster_time;
	rwLock(UNLOCK,&player_arr[id].rwlock);

	return ret;		
}



/*************************************************************
Aplica op player_arr[id].fied
op = RST = 0 -> poe o campo a 0
op = INC = 1 -> Incrementa o valor do campo
op = DEC = 2 -> Derementa o valor do campo
*************************************************************/
int get_field(char field,int id)
{
	int ret = 0;
	switch (field)
	{
		case SET_SCORE:

			rwLock(R_LOCK,&player_arr[id].rwlock);
			ret = player_arr[id].score;
			rwLock(UNLOCK,&player_arr[id].rwlock);
			break;
		case SET_EAT:

			rwLock(R_LOCK,&player_arr[id].rwlock);
			ret = player_arr[id].monster_eaten;
			rwLock(UNLOCK,&player_arr[id].rwlock);
			break;	
		case SET_SPEED_P:

			rwLock(R_LOCK,&player_arr[id].rwlock);
			ret = player_arr[id].speed_pac;
			rwLock(UNLOCK,&player_arr[id].rwlock);
			break;
		case SET_SPEED_M:

			rwLock(R_LOCK,&player_arr[id].rwlock);
			ret = player_arr[id].speed_monster;
			rwLock(UNLOCK,&player_arr[id].rwlock);
			break;	
		case GET_CLR:

			rwLock(R_LOCK,&player_arr[id].rwlock);
			ret = player_arr[id].color;
			rwLock(UNLOCK,&player_arr[id].rwlock);
			break;	
		case GET_FD:

			rwLock(R_LOCK,&player_arr[id].rwlock);
			ret = player_arr[id].player_fd;
			rwLock(UNLOCK,&player_arr[id].rwlock);
			break;
	}

	return ret;
}

int connected()
{
	int count=0;
	for (size_t i = 0; i < MAXPLAYERS; i++)
	{
		if(player_arr[i].playerID != UNAVAILABLE) count++;
	}
	
	return count;
}

// Funcao que remete as variaveis globais a zero (ou reset) e limpa o tabuleiro monitor
// (havia um bug em que os 2 ultimos pacman e monstro ficavam la presos por causa do accept)
void reset_all()
{
	if(!connected())
	{
		_nplayers(DEC);
		char elem;
		board_cell cell = {.id = UNAVAILABLE, .color = 0,.element = EMPTY};
		mutex(LOCK,&global_mutex);
		NUMPLAYERS = 0;			
		MorP = 0;
		LIVRES = 0;
		N_FRUITS = 0;
		N_BRICKS = 0;
		mutex(UNLOCK,&global_mutex);


		for (size_t i = 0; i < rows; i++)
		{
			for (size_t j = 0; j < cols; j++)
			{
				elem = get_cell_type(game_board,i,j);
				if ((elem != BRICK))
				{
					cell.coord.row = i;
					cell.coord.col = j;

					clear_cell(i,j);
					render_cell(&cell);
				}
			}
		}
		count_elements(game_board);	
	}
}


/*************************************************************
Funcao que gera uma posicao aleatoria para o monstro e pacman
do jogador recem conectado
*************************************************************/
void generate_coords(int id,board_cell *_monster, board_cell *_pacman)
{
		int mr,mc,pr,pc;
		int color = get_vect_color(player_arr,id); // Get character's color

		// Spawn monster position
		generate_pos(&(_monster->coord));
		get_coord(_monster,&mr,&mc);
		
		_monster->color = color;
		_monster->element = MONSTER;
		_monster->id = id;

		rwLock(W_LOCK,&game_board[mr][mc].rwlock);
		game_board[mr][mc].element = MONSTER;
		game_board[mr][mc].color = color;
		game_board[mr][mc].id = id;
		rwLock(UNLOCK,&game_board[mr][mc].rwlock);

		// Spawn pacman position
		generate_pos(&(_pacman->coord));
		get_coord(_pacman,&pr,&pc);
		_pacman->color = color;
		_pacman->element = PACMAN;
		_pacman->id = id;

		
		rwLock(W_LOCK,&game_board[pr][pc].rwlock);
		game_board[pr][pc].element = PACMAN;
		game_board[pr][pc].color = color;
		game_board[pr][pc].id = id;
		rwLock(UNLOCK,&game_board[pr][pc].rwlock);	
}
