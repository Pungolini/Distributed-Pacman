#include "board.h"


/********* GLOBALS *********/
unsigned short NUMPLAYERS = 0;
unsigned short N_FRUITS = 0;
unsigned short N_BRICKS = 0;
unsigned short N_PLAYERS = 0;
unsigned short LIVRES = 0;
unsigned short MorP = 0;
BOARD game_board;
int cols = 0 , rows = 0;

sem_t fruit_sem;
pthread_mutex_t global_mutex;
pthread_mutex_t pacman_mutex;
pthread_mutex_t monster_mutex;
pthread_rwlock_t nplayers_lock;
pthread_rwlock_t globals_lock;
/****************************/


/***********************************************************************/
// Permite devolver, de forma sincronizada
// o valor da variavel global LIVRES, que guarda o numero de 
// celulas vazias no tabuleiro a cada instante
/***********************************************************************/
short get_empty_cells()
{
	rwLock(R_LOCK,&globals_lock);
	short ret = LIVRES;
	rwLock(UNLOCK,&globals_lock);

	return ret;
}


/***********************************************************************/
// Permite incrementar, decrementar ou devolver, de forma sincronizada
// o valor da variavel global NUMPLAYERS, que guarda o numero de 
// jogadores conectados a cada instante
// op = INC -> INCrementa em uma unidade a variavel
// op = DEC -> DECrementa em uma unidade a variavel
// op = RET -> Devolve o valor da variavel
/***********************************************************************/
short _nplayers(char op)
{
	short ret = 0;

	if(op == RET)
	{
		rwLock(R_LOCK,&nplayers_lock);
		ret = NUMPLAYERS;
		rwLock(UNLOCK,&nplayers_lock);
	}
	else if(op == INC)
	{
		rwLock(W_LOCK,&nplayers_lock);
		ret = ++NUMPLAYERS;
		rwLock(UNLOCK,&nplayers_lock);
	}

	else if(op == DEC)
	{
		rwLock(W_LOCK,&nplayers_lock);
		ret = --NUMPLAYERS;
		rwLock(UNLOCK,&nplayers_lock);
	}

	return ret;
}

BOARD alloc_board(int _rows,int _cols)
{
    BOARD board_game = malloc((_rows)*sizeof(&board_game));
    checkMem(&board_game);

    for (size_t i = 0; i < _rows; i++)
    {
        board_game[i] =  calloc(_cols,sizeof(board_cell));
        checkMem(board_game[i]);
		for(int j = 0 ; j < _cols; j++)
		{
			board_game[i][j].coord.row = i;
			board_game[i][j].coord.col = j;

		}
    }
    return board_game;
}


void free_board()
{
	mutex(LOCK,&global_mutex);
	for(int i = 0; i < rows; i++)
	{
		for(int j = 0 ; j < cols; j++)
			rwLock(DESTROY,&game_board[i][j].rwlock);
	}
	mutex(UNLOCK,&global_mutex);
	

    for (size_t i = 0; i < rows; i++){
        free(game_board[i]);
	}
	
	free(game_board);
}


void printMat(BOARD M,int rows,int cols)
{
	printf("rows = %d\tcols = %d\n",rows,cols);
	for (int i = 0; i < rows ; ++i)
	{
		for (int j = 0; j < cols; ++j)
		{
			printf("%c  ",M[i][j].element );;
		}puts("\n");
	}
}

void show_cell(board_cell *st)
{
	puts("------playerStruct contents-----");
 	printf("color = %d\n",st->color);
	printf("elem = %c\n",st->element);
	printf("ID = %u\n",st->id);
	printf("pos = (%d  %d)\n",st->coord.row,st->coord.col);
	printf("Score = %d\n",player_arr[st->id].score);
	printf("Monster eaten = %d\n",player_arr[st->id].monster_eaten);
	puts("--------------------------------");

}

//Servidor
BOARD init_board(char *file)
{
    FILE *fpIn = fopen(file,"r");
    checkMem(fpIn);

    
    if (fscanf(fpIn,"%d %d", &cols,&rows) != 2) exit(-1);

    // Alocar memoria para vetor rows x cols
    BOARD my_board = alloc_board(rows,cols);


    // Guardar caracteres na matriz
    char *buffer = malloc(cols*sizeof(char));
    checkMem(buffer);

	char ch;
    for(int i = 0;(i < rows ) && (!feof(fpIn)); i++)
    { 
        for(int j = 0; j < cols; j++)
		{
			ch = fgetc(fpIn);
			if(ch == '\n'){
				j--;
				continue;
			}
			my_board[i][j].element = (ch != ' ') && (ch != '\n') ? ch : EMPTY ;

			if(pthread_rwlock_init(&my_board[i][j].rwlock,NULL)) exit(-1);
			
		}

    }

    free(buffer);
    fclose(fpIn);

    return my_board;
}

/******* SINCRONIZACAO *******/
void rwLock(char op, pthread_rwlock_t* lock)
{
	int err = 0;

	switch (op){
		case R_LOCK:
			err = pthread_rwlock_rdlock(lock);
			break;
		case W_LOCK:
			err = pthread_rwlock_wrlock(lock);
			break;

		case UNLOCK:
			err = pthread_rwlock_unlock(lock);
			break;

		case INIT:
			err = pthread_rwlock_init(lock,NULL);
			break;
		case DESTROY:
			err = pthread_rwlock_destroy(lock);
			break;
	}

	if(err){
		exit(EXIT_FAILURE);
	}
}


void mutex(char op, pthread_mutex_t* lock)
{
	int err = 0;

	switch (op){
		case LOCK:
			err = pthread_mutex_lock(lock);
			break;

		case UNLOCK:
			err = pthread_mutex_unlock(lock);
			break;

		case INIT:
			err = pthread_mutex_init(lock,NULL);
			break;
		case DESTROY:
			err = pthread_mutex_destroy(lock);
			break;

	}

	if(err)
		exit(EXIT_FAILURE);
}

void init_sync()
{
	mutex(INIT,&global_mutex);

	mutex(INIT,&pacman_mutex);
	
	mutex(INIT,&monster_mutex);

	rwLock(INIT,&nplayers_lock);

	rwLock(INIT,&globals_lock);

	
	if(sem_init(&fruit_sem, 0, 0))
		exit(-1);
}

void count_elements(BOARD board)
{
	char elem;
	mutex(LOCK,&global_mutex);
	MorP = 0;
	LIVRES = 0;
	N_FRUITS = 0;
	N_BRICKS = 0;
	mutex(UNLOCK,&global_mutex);

	for (size_t i = 0; i < rows; i++)
	{	for (size_t j = 0; j < cols; j++)
		{

			elem = get_cell_type(board,i,j);
			mutex(LOCK,&global_mutex);
			switch(elem)
			{
				case PACMAN:
				case MONSTER:
				case POWERED:
					
					MorP++;
					break;
				case LEMON:
				case CHERRY:
					N_FRUITS++;
					break;
				case BRICK:
					N_BRICKS++;
					break;
				case EMPTY:
					LIVRES++;
					break;		
			}
			mutex(UNLOCK,&global_mutex);
		}
	}
			
}


int get_char_pos(board_cell *old_play,int id,char elem)
{
	int cell_id = 0;
	char cell_elem ;
	for (size_t i = 0; i < rows; i++)
		for (size_t j = 0; j < cols; j++)
		{
			rwLock(R_LOCK,&game_board[i][j].rwlock);
			cell_id = game_board[i][j].id;
			cell_elem = game_board[i][j].element;
			rwLock(UNLOCK,&game_board[i][j].rwlock);

			if( (id != INVALID) && (cell_id == id) && (cell_elem == elem))
			{
				rwLock(R_LOCK,&game_board[i][j].rwlock);
				*old_play = game_board[i][j];
				rwLock(UNLOCK,&game_board[i][j].rwlock);

				return 1;
			}else if( (id == INVALID) &&  is_fruit(cell_elem) ) // fruta
			{
				rwLock(R_LOCK,&game_board[i][j].rwlock);
				*old_play = game_board[i][j];
				rwLock(UNLOCK,&game_board[i][j].rwlock);
				return 1;
			}
			
		}
	return 0;
}


void generate_pos(point *pos)
{
	short n_cells = rows*cols;
	int x = rand()%(rows-1);
	int y = rand()%(cols-1);

	count_elements(game_board);

	int free_cells = get_empty_cells();

	if (free_cells > (n_cells/2)) // Metodo aleatorio
	{

		while (get_cell_type(game_board,x,y) != EMPTY)
		{
			x = rand()%(rows-1);
			y = rand()%(cols-1);
		}
		pos->row = x;
		pos->col = y;
	}

	else if (free_cells > ((n_cells)/5)) // metodo pseudo-aleatorio
	{
		puts("Metodo pseudo-aleatorio!");
		while (get_cell_type(game_board,x,y) != EMPTY)
		{
			x = rand()%(rows-1);
			for(y = 0; y < cols; y++)
			{
				if (get_cell_type(game_board,x,y) == EMPTY)
				{
					pos->row = x;
					pos->col = y;
					break;
				}
			}	
		}
	}
	else // Percorrer a matriz
	{
		int found = 0;
		puts("Metodo linear!\n");
		for(size_t i = 0; i < rows && !found; ++i)
		{
			for(size_t j = 0; j < cols; ++j)
			{
				if (get_cell_type(game_board,x,y) == EMPTY)
				{
					pos->row = i;
					pos->col = j;
					found = 1;
					break;	
				}		
			}
		}
	}
}

int same_cells(board_cell *c1,board_cell *c2)
{
	return (c1->coord.row == c2->coord.row) && (c1->coord.col == c2->coord.col);
}

int cells_are_adjacents(point *cell1,point *cell2)
{

	int dx = 0,dy = 0;

	dx = abs(cell1->row - cell2->row);
	dy = abs(cell1->col - cell2->col);

	return (dx + dy) == 1;
}


void monster_new_pos(board_cell *old_play,board_cell *pos)
{
	// Obter posicao nova depois da tecla ter sido pressionada
	if (is_monster(pos))
	{
		int dx = 0,dy = 0,key = pos->color;

		if (key == LEFT)
			dx = -1;
		else if (key == UP)
			dy = -1;
		else if(key == RIGHT)
			dx = 1;
		else if(key == DOWN)
			dy = 1;
		
		pos->coord.row = old_play->coord.row + dy;
		pos->coord.col = old_play->coord.col + dx;
	}
}
// Mete nas coordenadas de pos a informacao da jogada no tabuleiro
// Antes de chamar, preencher a struct pos
void update_cell(BOARD board,int id,board_cell *pos)
{
	int r,c;
	get_coord(pos,&r,&c);


	rwLock(W_LOCK,&game_board[r][c].rwlock);
	game_board[r][c].id = id;
	game_board[r][c].element = pos->element;
	game_board[r][c].color = pos->color;
	rwLock(UNLOCK,&game_board[r][c].rwlock);
}

void spawn_fruit(BOARD *board,int n_players,board_cell *fruit_pos)
{
	// Spawn fruit

	if (n_players > 1)
	{
		generate_pos(&(fruit_pos->coord));
		int r,c;
		get_coord(fruit_pos,&r,&c);

		char choice = (rand()%2 == 0) ? LEMON : CHERRY;
		
		set_cell_type(&game_board,choice,r,c);
		set_char(fruit_pos,choice);
	}
}


/********* CODIGO DE ABSTRACAO DOS DADOS ********/

/*************** PlayerStruct *******************/
void set_player_color(playerStruct* p,int color)
{
	checkMem(p);

	p->color = color;
}

void set_player_fd(playerStruct* p,int fd)
{
	checkMem(p);

	p->player_fd = fd;
}

void set_player_id(playerStruct* p,int id)
{
	checkMem(p);

	p->playerID = id;
}

int get_player_fd(playerStruct *p)
{
	return (p) ? p->player_fd : INVALID;
}


int get_player_color(playerStruct *p)
{
	return (p) ? p->color : INVALID;
}


int get_player_id(playerStruct *p)
{
	return (p) ? p->playerID: INVALID;
}

int get_player_score(playerStruct *p)
{
	return (p) ? p->score: INVALID;
}

int get_monster_eaten(playerStruct *p)
{
	return (p) ? p->monster_eaten: INVALID;
}







/******* API do tabuleiro*********/



/************** ABSTRACAO*****************************/
void set_cell_id(BOARD *board,int id,int r,int c)
{
	rwLock(W_LOCK,&game_board[r][c].rwlock);
	game_board[r][c].id = id;
	rwLock(UNLOCK,&game_board[r][c].rwlock);
	
}

void set_cell_type(BOARD* board,char elem,int r,int c)
{
	rwLock(W_LOCK,&game_board[r][c].rwlock);
	game_board[r][c].element = elem;
	rwLock(UNLOCK,&game_board[r][c].rwlock);
	
	switch(elem)
	{
		case PACMAN:
		case MONSTER:
			MorP++;
			break;
		case LEMON:
		case CHERRY:
			N_FRUITS++;
			break;
		case BRICK:
			N_BRICKS++;
			break;		
	}
	if (elem != EMPTY) LIVRES--;
	else LIVRES++;

}

void set_cell_clr(BOARD* board,int color,int r,int c)
{
	rwLock(W_LOCK,&game_board[r][c].rwlock);
	game_board[r][c].color = color;
	rwLock(UNLOCK,&game_board[r][c].rwlock);
}

char get_cell_type(BOARD board,int r,int c)
{
	rwLock(R_LOCK,&board[r][c].rwlock);
	char ret = game_board[r][c].element;
	rwLock(UNLOCK,&board[r][c].rwlock);
	return ret;
}

int get_cell_clr(BOARD board,int r,int c)
{
	
	rwLock(R_LOCK,&board[r][c].rwlock);
	int ret = game_board[r][c].color;
	rwLock(UNLOCK,&board[r][c].rwlock);
	return ret;
}

int get_cell_id(BOARD board,int r,int c)
{
	rwLock(R_LOCK,&board[r][c].rwlock);
	int ret = game_board[r][c].id;
	rwLock(UNLOCK,&board[r][c].rwlock);
	return ret;
}

board_cell* get_cell_ptr(BOARD board,int r,int c)
{
	rwLock(R_LOCK,&board[r][c].rwlock);
	board_cell *ptr = &game_board[r][c];
	rwLock(UNLOCK,&board[r][c].rwlock);

	return ptr;
}

int cell_is_brick(BOARD board,int r,int c)
{
	return get_cell_type(board,r,c) == BRICK;
}

int cell_is_fruit(BOARD board,int r,int c)
{
	char elem =  get_cell_type(board,r,c);
	return (elem == CHERRY) || (elem == LEMON) ;
}

int cell_is_character(BOARD board,int r,int c)
{
	char elem =  get_cell_type(board,r,c);
	return (elem == PACMAN) || (elem == MONSTER) ;
}


int cell_is_mine(BOARD board,int id,int r, int c )
{
	return get_cell_id(board,r,c) == id;
}

int row_is_valid(board_cell *x)
{
	 return (x->coord.row >= 0 ) && (x->coord.row < rows);
}

int col_is_valid(board_cell * x) 
{
	return (x->coord.col >= 0 ) && (x->coord.col < cols);
}

int cell_is_valid(board_cell * x) 
{
	return row_is_valid(x) && col_is_valid(x);
}
/*------------------- GAMEPLAY ---------------------*/


void swap_positions(board_cell *char1,board_cell *char2)
{

	board_cell tmp = *char1;

	// Meter conteudo de char2 em char1, exceto a posicao
	char1->element = char2->element;
	char1->color = char2->color;
	char1->id = char2->id;

	// Meter conteudo de tmp em char2, exceto a posicao
	char2->element = tmp.element;
	char2->id = tmp.id;
	char2->color = tmp.color;

}
/* 0 - BOUNCE
   1 - EAT FRUIT
   2 - EAT CHAR
   3 - STEP (normal move)
   -1 - Not valid
*/
int play_is_valid(BOARD board,int playerID,board_cell *old_play,board_cell *new_play)
{
	board_cell dummy = *new_play;

	// Obter posicao nova depois da tecla ter sido pressionada
	monster_new_pos(old_play,&dummy);

	// Devolver proxima jogada

	if (cell_is_valid(&dummy)) // Nova jogada esta dentro do tabuleiro e adjacente a antiga
	{
;
		if(cells_are_adjacents(&old_play->coord,&dummy.coord))
		{
			int r,c;
			get_coord(&dummy,&r,&c);
			char board_elem = get_cell_type(game_board,r,c);
			new_play->coord = dummy.coord;

			switch (board_elem)
			{
				case BRICK:
					return BOUNCE;
				
				case EMPTY:
					return STEP;

				case LEMON:
				case CHERRY:
					return EAT_FRUIT;
				
				case POWERED:
				case PACMAN:
				case MONSTER:
					return EAT_CHAR;
				
				default:
					return INVALID;
			}
		}

	}else return BOUNCE;

	
	return INVALID;	
}

int bounce(board_cell *old_play,board_cell *play,int dir)
{
	int r,c;
	char elem;
	board_cell dummy = *play;

	monster_new_pos(old_play,&dummy);

	// Fazer bounce numa brick
	if (cell_is_valid(&dummy))
	{
		// Nova posicao, depois do bounce
		dummy.coord.row = old_play->coord.row + (old_play->coord.row - play->coord.row);
		dummy.coord.col = old_play->coord.col + (old_play->coord.col - play->coord.col);
	}

	if (!cell_is_valid(&dummy))
	{
		dummy = *old_play;
		get_coord(&dummy,&r,&c);
		
		// Verificar se a posicao atual + a direcao e uma celula valida. Se nao for, fazer bounce
		if (is_monster(play))
		{
			if(is_last_col(dummy) && (dir == RIGHT))
				dummy.coord.col = cols - 2;

			else if(is_last_row(dummy) && (dir == DOWN))
				dummy.coord.row = rows - 2;

			else if(!old_play->coord.col && (dir == LEFT))
				dummy.coord.col = 1;
			
			else if(!old_play->coord.row && (dir == UP))
			{
				dummy.coord.row = 1;
				dummy.coord.col = c;
			}
		}
		else 
		{
			// Fazer bounce do pacman nos limites direitos e inferior (nos outros nao ha forma de saber se o rato saiu do tabuleiro)
			if(is_last_row(dummy) && !is_last_col(dummy) )
				dummy.coord.row -= 1;

			else if(is_last_col(dummy) && !is_last_row(dummy))
				dummy.coord.col -= 1;
			
			else if(is_last_row(dummy) &&is_last_col(dummy) ) // canto
			{
				if(!row_is_valid(play))
					dummy.coord.row -= 1;

				else if(!col_is_valid(play))
					dummy.coord.col -= 1;
			}

		}
	}
	//So dar bounce quando nao se esta preso entre 2 blocos
	// Ou se ao der bounce caimos em cima de uma personagem
	get_coord(&dummy,&r,&c);
	elem = get_cell_type(game_board,r,c);
	if(cell_is_valid(&dummy) &&((elem == EMPTY) || is_fruit(elem)))
	{
		if(is_fruit(elem))
			return 1;
		play->coord = dummy.coord;
		return 2;	
	}
	return 0;
}

int chars_are_equal(char c1,char c2)
{
	int both_are_pacman = ((c1 == PACMAN) && (c2 == PACMAN)) ||
						  ((c1 == PACMAN) && (c2 == POWERED)) ||
						  ((c1 == POWERED) && (c2 == PACMAN)) ||
						  ((c1 == POWERED) && (c2 == POWERED));

	int both_are_ghosts = (c1 == MONSTER) && (c2 == MONSTER);

	return both_are_pacman || both_are_ghosts;
						
}

void get_curr_pos(int id,board_cell *old_play,board_cell *new_play)
{
	if (is_monster(new_play))
		get_char_pos(old_play,id,MONSTER);

	else if (is_pacman(new_play))
	{
		if(!get_char_pos(old_play,id,PACMAN)) // Buscar posicao do (powered)pacman
		{
			if(get_char_pos(old_play,id,POWERED))
			{
				set_char(new_play,POWERED);
				set_char(old_play,POWERED);
			}
		}
	}
}
void clear_cell(int r,int c)
{
	rwLock(W_LOCK,&game_board[r][c].rwlock);
	game_board[r][c].id = -1;
	game_board[r][c].color = 0;
	game_board[r][c].element = EMPTY;
	rwLock(UNLOCK,&game_board[r][c].rwlock);
}

