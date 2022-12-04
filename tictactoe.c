#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct State {
	char grid[3][3];
	char player, opponent, empty;
};

/* Game */
void play_human(struct State *state);
void play_self(struct State *state);
void print_grid(struct State *state);
void end_turn(struct State *state);
bool is_game_over(struct State *state);

/* Minimax */
void decide(struct State *state, int *row, int *col, int depth);
int maximize(struct State *state, int depth);
int minimize(struct State *state, int depth);
bool is_grid_full(struct State *state);
bool is_illegal_move(struct State *state, int row, int col);

/* Evaluation */
int evaluate(struct State *state);
int evaluate_horizontal(struct State *state, int row);
int evaluate_vertical(struct State *state, int col);
int evaluate_diagonal(struct State *state);

int main(int argc, char *argv[]) {
	struct State initial = {
		{{' ', ' ', ' '},
		 {' ', ' ', ' '},
		 {' ', ' ', ' '}}, 
		'X', 'O', ' '
	};

	if (argc > 1) {
		if (strcmp(argv[1], "--self") == 0) {
			play_self(&initial);
			return 0;
		}
	}

	play_human(&initial);

	return 0;
}

void play_human(struct State *state) {
	int row, col;

	while (!is_game_over(state)) {
		row = -1, col = -1;
		do {
			printf("Your move (row, col): ");
			scanf("%d,%d", &row, &col);
		} while (is_illegal_move(state, row - 1, col - 1));

		state->grid[row - 1][col - 1] = state->player;
		print_grid(state);
		end_turn(state);
		printf("\n");

		if(is_game_over(state))
			break;

		decide(state, &row, &col, 8);
		state->grid[row][col] = state->player;
		printf("%c plays (%d, %d)\n", state->player, row, col);
		print_grid(state);
		end_turn(state);
		printf("\n");
	}
}

void play_self(struct State *state) {
	int row, col, score;

	srand((unsigned)time(NULL));
	row = rand() % 3, col = rand() % 3;
	state->grid[row][col] = state->player;
	printf("%c plays (%d, %d)\n", state->player, row, col);
	print_grid(state);
	end_turn(state);
	printf("\n");

	while(!is_game_over(state)) {
		decide(state, &row, &col, 8);
		state->grid[row][col] = state->player;
		printf("%c plays (%d, %d)\n", state->player, row, col);
		print_grid(state);
		end_turn(state);
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
		for (col = 0; col < 3; ++col) {
			printf("[%c]", state->grid[row][col]);
		}
		printf("\n");
	}
}

void end_turn(struct State *state) {
	char temp;

	temp = state->player;
	state->player = state->opponent;
	state->opponent = temp;
}

bool is_game_over(struct State *state) {
	return (is_grid_full(state) || evaluate(state));
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

int maximize(struct State *state, int depth) {
	int row, col, value, max_value;
	bool is_terminal;

	is_terminal = ((value = evaluate(state)) || is_grid_full(state));
	if (is_terminal || depth == 0)
		return value;

	max_value = -10;
	for (row = 0; row < 3; ++row) {
		for (col = 0; col < 3; ++col) {
			if (is_illegal_move(state, row, col))
				continue;
			state->grid[row][col] = state->player;
			value = minimize(state, depth - 1);
			state->grid[row][col] = state->empty;
			if (value > max_value)
				max_value = value;
		}
	}

	return max_value;
}

int minimize(struct State *state, int depth) {
	int row, col, value, min_value;
	bool is_terminal;

	is_terminal = ((value = evaluate(state)) || is_grid_full(state));
	if (is_terminal || depth == 0)
		return value;

	min_value = 10;
	for (row = 0; row < 3; ++row) {	
		for (col = 0; col < 3; ++col) {
			if (is_illegal_move(state, row, col))
				continue;
			state->grid[row][col] = state->opponent;
			value = maximize(state, depth - 1);
			state->grid[row][col] = state->empty;
			if (value < min_value)
				min_value = value;
		}
	}

	return min_value;
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

	if (row < 0 || row > 2)
		return 0;

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
