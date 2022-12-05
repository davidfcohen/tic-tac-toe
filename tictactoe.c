#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>

struct State {
	char grid[3][3];
	char player, opponent, empty;
};

int g_depth, g_nodes;
bool g_prune, g_verbose;

/* Game */

void play_human(struct State *state);
void play_self(struct State *state);
void print_grid(struct State *state);
void move(struct State *state, int row, int col);
void decide(struct State *state, int *best_row, int *best_col, int depth);
void end_turn(struct State *state);

/* Minimax */

int maximize(struct State *state, int depth);
int minimize(struct State *state, int depth);
int maximize_prune(struct State* state, int depth, int alpha, int beta);
int minimize_prune(struct State* state, int depth, int alpha, int beta);

/* Evaluation */

int evaluate(struct State *state);
int evaluate_horizontal(struct State *state, int row);
int evaluate_vertical(struct State *state, int col);
int evaluate_diagonal(struct State *state);

/* Helper Functions */

bool is_game_over(struct State *state);
bool is_grid_full(struct State *state);
bool is_illegal_move(struct State *state, int row, int col);
int min(int a, int b);
int max(int a, int b);

int main(int argc, char *argv[]) {
	int opt;
	bool self;

	struct State initial = {
		{{' ', ' ', ' '},
		 {' ', ' ', ' '},
		 {' ', ' ', ' '}}, 
		'X', 'O', ' '
	};

	g_depth = 7;
	while((opt = getopt(argc, argv, "spd:vh")) != -1) {
		switch (opt) {
			case 's':
				self = true;
				break;
			case 'p':
				g_prune = true;
				break;
			case 'd':
				if (*optarg  < '0' || *optarg > '7') {
					printf("The depth argument must be between 1 and 7 inclusive.\n");
					return 0;
				}
				g_depth = atoi(optarg);
				break;
			case 'v':
				g_verbose = true;
				break;
			case 'h':
			case '?':
				printf("%s [-s] [-p] [-d depth] [-v]\n", argv[0]);
				printf("\t-s: Simulate a full game of tic-tac-toe.\n");
				printf("\t-p: Use alpha-beta pruning.\n");
				printf("\t-d: Set the maximum depth of the decision tree (0-7).\n");
				printf("\t-v: Output the number of expanded nodes.\n");
				return 0;
		}
	}

	if (self) {
		play_self(&initial);
		return 0;
	}

	play_human(&initial);
	return 0;
}

/* Game */

void play_human(struct State *state) {
	int row, col;
	char ch;

	printf("\n");
	print_grid(state);
	printf("\n");
	while (!is_game_over(state)) {
		row = -1, col = -1;
		do {
			printf("Your move: ");
			while((ch = getchar()) != '\n') {
				col = ch - 'a';
				ch = tolower(getchar());
				row = (ch - '0' - 3) * -1;
			}
		} while (is_illegal_move(state, row, col));
		move(state, row, col);
		print_grid(state);
		end_turn(state);
		printf("\n");

		if(is_game_over(state))
			break;

		decide(state, &row, &col, g_depth);
		move(state, row, col);
		printf("%c plays %c%c:\n", state->player, col + 'a', (row - 3) * -1);
		print_grid(state);
		end_turn(state);
		if (g_verbose) {
			printf("Minimax expanded %d nodes for this move.\n", g_nodes);
			g_nodes = 0;
		}
		printf("\n");
	}
}

void play_self(struct State *state) {
	int row, col, score;

	srand((unsigned)time(NULL));
	row = rand() % 3, col = rand() % 3;
	move(state, row, col);
	printf("%c plays %c%d:\n", state->player, col + 'a', (row - 3) * -1);
	print_grid(state);
	end_turn(state);
	printf("\n");

	while(!is_game_over(state)) {
		decide(state, &row, &col, g_depth);
		move(state, row, col);
		printf("%c plays %c%d:\n", state->player, col + 'a', (row - 3) * -1);
		print_grid(state);
		end_turn(state);
		if (g_verbose) {
			printf("Minimax expanded %d nodes.\n", g_nodes);
			g_nodes = 0;
		}
		printf("\n");
	}

	score = evaluate(state);
	if (score > 0)
		printf("%c wins!\n", state->player);
	else if (score < 0)
		printf("%c wins!\n", state->opponent);
	else
		printf("Tie!\n");
}

void print_grid(struct State *state) {
	int row, col;

	for (row = 0; row < 3; ++row) {
		printf("%d ", (row - 3) * -1);
		for (col = 0; col < 3; ++col) {
			printf("[%c]", state->grid[row][col]);
		}
		printf("\n");
	}
	printf("   a  b  c\n");
}

void move(struct State *state, int row, int col) {
	state->grid[row][col] = state->player;
}

void end_turn(struct State *state) {
	char temp;

	temp = state->player;
	state->player = state->opponent;
	state->opponent = temp;
}

void decide(struct State *state, int *best_row, int *best_col, int depth) {
	int row, col, value, best_value;
	bool is_first_availible_move;

	best_value = -10, is_first_availible_move = true;
	for (row = 0; row < 3; ++row) {
		for (col = 0; col < 3; ++col) {
			if (is_illegal_move(state, row, col))
				continue;
			if (is_first_availible_move) {
				*best_row = row;
				*best_col = col;
				is_first_availible_move = false;
			}
			state->grid[row][col] = state->player;
			if (g_prune)
				value = minimize_prune(state, depth, -10, 10);
			else
				value = minimize(state, depth);
			state->grid[row][col] = state->empty;
			if (value > best_value) {
				best_value = value;
				*best_row = row;
				*best_col = col;
			}
		}
	}
}

/* Minimax */

int maximize(struct State *state, int depth) {
	int row, col, value, max_value;
	
	if(g_verbose)
		++g_nodes;

	if (is_game_over(state) || depth == 0)
		return evaluate(state);

	max_value = -10;
	for (row = 0; row < 3; ++row) {
		for (col = 0; col < 3; ++col) {
			if (is_illegal_move(state, row, col))
				continue;
			state->grid[row][col] = state->player;
			value = minimize(state, depth - 1) - depth;
			state->grid[row][col] = state->empty;
			max_value = max(max_value, value);
		}
	}

	return max_value;
}

int minimize(struct State *state, int depth) {
	int row, col, value, min_value;

	if(g_verbose)
		++g_nodes;

	if (is_game_over(state) || depth == 0)
		return evaluate(state);

	min_value = 10;
	for (row = 0; row < 3; ++row) {	
		for (col = 0; col < 3; ++col) {
			if (is_illegal_move(state, row, col))
				continue;
			state->grid[row][col] = state->opponent;
			value = maximize(state, depth - 1) + depth;
			state->grid[row][col] = state->empty;
			min_value = min(min_value, value);
		}
	}

	return min_value;
}

int maximize_prune(struct State* state, int depth, int alpha, int beta) {
	int row, col, value, max_value;
	
	if(g_verbose)
		++g_nodes;

	if (is_game_over(state) || depth == 0)
		return evaluate(state);

	max_value = -10;
	for (row = 0; row < 3; ++row) {
		for (col = 0; col < 3; ++col) {
			if (is_illegal_move(state, row, col))
				continue;
			state->grid[row][col] = state->player;
			value = minimize_prune(state, depth - 1, alpha, beta) - depth;
			state->grid[row][col] = state->empty;
			max_value = max(max_value, value);
			if (value >= beta)
				return max_value;
			alpha = max(alpha, value);
		}
	}

	return max_value;
}

int minimize_prune(struct State* state, int depth, int alpha, int beta) {
	int row, col, value, min_value;

	if(g_verbose)
		++g_nodes;

	if (is_game_over(state) || depth == 0)
		return evaluate(state);

	min_value = 10;
	for (row = 0; row < 3; ++row) {	
		for (col = 0; col < 3; ++col) {
			if (is_illegal_move(state, row, col))
				continue;
			state->grid[row][col] = state->opponent;
			value = maximize_prune(state, depth - 1, alpha, beta) + depth;
			state->grid[row][col] = state->empty;
			min_value = min(min_value, value);
			if (value <= alpha)
				return min_value;
			beta = min(beta, value);
		}
	}

	return min_value;
}

/* Evaluation */

int evaluate(struct State *state) {
	int i, value;

	for (i = 0; i < 3; ++i) {
		if ((value = evaluate_horizontal(state, i)))
			return value;
	}

	for (i = 0; i < 3; ++i) {
		if ((value = evaluate_vertical(state, i))) {
			return value;
		}
	}

	return evaluate_diagonal(state);
}

int evaluate_horizontal(struct State *state, int row) {
	bool is_win, is_loss;

	is_win = (state->player == state->grid[row][0] && 
			  state->player == state->grid[row][1] && 
			  state->player == state->grid[row][2]);
	
	is_loss = (state->opponent == state->grid[row][0] &&
			   state->opponent == state->grid[row][1] &&
			   state->opponent == state->grid[row][2]);

	if(is_win)
		return 10;
	
 	if (is_loss)
 		return -10;

	return 0;
}

int evaluate_vertical(struct State *state, int col) {
	bool is_win, is_loss;

	is_win = (state->player == state->grid[0][col] && 
			  state->player == state->grid[1][col] && 
		   	  state->player == state->grid[2][col]);
	
	is_loss = (state->opponent == state->grid[0][col] &&
			   state->opponent == state->grid[1][col] &&
			   state->opponent == state->grid[2][col]);

	if(is_win)
		return 10;
	
 	if (is_loss)
 		return -10;

	return 0;
}

int evaluate_diagonal(struct State *state) {
	bool is_win, is_loss;

	is_win = ((state->player == state->grid[0][0] &&
			   state->player == state->grid[1][1] &&
			   state->player == state->grid[2][2]) ||
			  (state->player == state->grid[0][2] &&
			   state->player == state->grid[1][1] &&
			   state->player == state->grid[2][0]));

	is_loss = ((state->opponent == state->grid[0][0] &&
			   state->opponent == state->grid[1][1] &&
			   state->opponent == state->grid[2][2]) ||
			  (state->opponent == state->grid[0][2] &&
			   state->opponent == state->grid[1][1] &&
			   state->opponent == state->grid[2][0]));

	if (is_win)
		return 10;

	if (is_loss)
		return -10;

	return 0;
}

/* Helper Functions */

bool is_game_over(struct State *state) {
	return (is_grid_full(state) || evaluate(state));
}

bool is_illegal_move(struct State* state, int row, int col) {
	bool is_empty;

	is_empty = state->grid[row][col] == state->empty;
	return (!is_empty || row < 0 || col < 0 || row > 2 || row > 2);
}

bool is_grid_full(struct State *state) {
	int row, col;

	for (row = 0; row < 3; ++row)
		for (col = 0; col < 3; ++col)
			if (state->grid[row][col] == state->empty)
				return false;
	return true;
}

int max(int a, int b) {
	if (a > b)
		return a;
	return b;
}

int min(int a, int b) {
	if (a < b)
		return a;
	return b;
}
