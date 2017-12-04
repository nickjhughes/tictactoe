#include <iostream>
#include <map>
#include <random>
#include <vector>

/*
TODO:
    - Masks for checking for wins/full board
    - Optimal move generation
    - Auto-play
*/

/*
Game state is stored in a 32 bit integer:
    - the first 18 bits are the values of each position
    - row-major from top-left
    - (00 = empty, 01 = cross, 10 = nought)
    - the 19th bit is 0 for cross' turn and 1 for nought's
*/

using namespace std;

typedef __uint32_t state_t;
typedef char symbol_t;
typedef char position_t;
typedef signed char value_t;

const symbol_t empty = 0x0;
const symbol_t x = 0x1;
const symbol_t o = 0x3;
const symbol_t both = 0x4;
const state_t pos_masks[9] = {0xC0000000, 0x30000000, 0x0C000000,
                              0x03000000, 0x00C00000, 0x00300000,
                              0x000C0000, 0x00030000, 0x0000C000};
const char pos_shifts[9] = {30, 28, 26, 24, 22, 20, 18, 16, 14};
const state_t turn_mask = 0x00002000;
const char turn_shift = 13;

map<state_t, position_t> optimal_moves;
map<state_t, value_t> optimal_values;

void reset_state(state_t &state) {
    state = 0;
}

void swap_turn(state_t &state) {
    state ^= turn_mask;
}

symbol_t whose_turn(const state_t &state) {
    return (((state >> turn_shift) & 0x1) == 0) ? x : o;
}

void set_pos(state_t &state, position_t pos, symbol_t symbol) {
    state |= (pos_masks[pos] & (symbol << pos_shifts[pos]));
}

symbol_t get_pos(const state_t &state, position_t pos) {
    return (state >> pos_shifts[pos]) & 0x3;
}

symbol_t check_row(const state_t &state, char row) {
    if (get_pos(state, row * 3) != empty &&
        get_pos(state, row * 3) == get_pos(state, row * 3 + 1) &&
        get_pos(state, row * 3 + 1) == get_pos(state, row * 3 + 2)) {
        return get_pos(state, row * 3);
    } else {
        return 0;
    }
}

symbol_t check_column(const state_t &state, char col) {
    if (get_pos(state, col) != empty &&
        get_pos(state, col) == get_pos(state, col + 3) &&
        get_pos(state, col + 3) == get_pos(state, col + 2 * 3)) {
        return get_pos(state, col);
    } else {
        return 0;
    }
}

symbol_t check_diagonal(const state_t &state, char diag) {
    if (diag == 0) {
        if (get_pos(state, 0) != empty &&
            get_pos(state, 0) == get_pos(state, 4) &&
            get_pos(state, 4) == get_pos(state, 8)) {
            return get_pos(state, 0);
        }
    } else {
        if (get_pos(state, 2) != empty &&
            get_pos(state, 2) == get_pos(state, 4) &&
            get_pos(state, 4) == get_pos(state, 6)) {
            return get_pos(state, 2);
        }
    }
}

char num_symbols(const state_t &state) {
    char sum = 0;
    for (position_t pos = 0; pos < 9; ++pos) {
        sum += get_pos(state, pos) != empty ? 1 : 0;
    }
    return sum;
}

bool board_full(const state_t &state) {
    return num_symbols(state) == 9;
}

symbol_t game_over(const state_t &state) {
    for (char row = 0; row < 3; ++row) {
        symbol_t winner = check_row(state, row);
        if (winner != 0) {
            return winner;
        }
    }
    for (char col = 0; col < 3; ++col) {
        symbol_t winner = check_column(state, col);
        if (winner != 0) {
            return winner;
        }
    }
    for (char diag = 0; diag < 2; ++diag) {
        symbol_t winner = check_diagonal(state, diag);
        if (winner != 0) {
            return winner;
        }
    }
    
    if (board_full(state)) {
        // Draw
        return both;
    }
    
    return 0;
}

void enumerate_moves(const state_t &state) {
    if (optimal_moves.count(state) == 1 || optimal_values.count(state) == 1) {
        return;
    }
    
    symbol_t game_state = game_over(state);
    if (game_state != 0) {
        optimal_moves[state] = -1;
        if (game_state == both) {
            optimal_values[state] = 0;
        } else if (game_state == whose_turn(state)) {
            optimal_values[state] = 1;
        } else {
            optimal_values[state] = -1;
        }
        return;
    }
    
    position_t best_pos;
    value_t best_value = -2;
    for (position_t pos = 0; pos < 9; ++pos) {
        if (get_pos(state, pos) != empty) {
            continue;
        }
        state_t new_state = state;
        set_pos(new_state, pos, whose_turn(new_state));
        swap_turn(new_state);
        enumerate_moves(new_state);
        
        value_t value = -optimal_values[new_state];
        if (value > best_value) {
            best_value = value;
            best_pos = pos;
        }
    }
    optimal_values[state] = best_value;
    optimal_moves[state] = best_pos;
}

void take_random_turn(state_t &state) {
    vector<position_t> empty_squares;
    for (position_t pos = 0; pos < 9; ++pos) {
        if (get_pos(state, pos) == empty) {
            empty_squares.push_back(pos);
        }
    }
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, empty_squares.size() - 1);
    position_t square = empty_squares[dis(gen)];
    set_pos(state, square, whose_turn(state));
}

void take_optimal_turn(state_t &state) {
    set_pos(state, optimal_moves[state], whose_turn(state));
}

void print_state(const state_t &state) {
    for (position_t pos = 0; pos < 9; ++pos) {
        symbol_t symbol = ' ';
        symbol_t at_pos = get_pos(state, pos);
        if (at_pos == x) {
            symbol = 'X';
        } else if (at_pos == o) {
            symbol = 'O';
        }
        cout << symbol;
        if ((pos + 1) % 3 == 0) {
            cout << endl;
        } else {
            cout << "|";
        }
    }
    cout << endl;
}

symbol_t play_game(bool print) {
    state_t state;
    reset_state(state);
    while (game_over(state) == 0) {
        // take_random_turn(state);
        take_optimal_turn(state);
        swap_turn(state);
        if (print) {
            print_state(state);
        }
    }
    if (print) {
        if (game_over(state) == x) {
            cout << "Crosses won!" << endl;
        } else if (game_over(state) == o) {
            cout << "Noughts won!" << endl;
        } else if (game_over(state) == both) {
            cout << "Draw!" << endl;
        }
    }
    return game_over(state);
}

int main() {
    // symbol_t winner = play_game(true);

    state_t state;
    reset_state(state);
    enumerate_moves(state);
    symbol_t winner = play_game(true);

    cout << to_string(optimal_moves.size()) << endl;
    
    return 0;
}
