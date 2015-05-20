
#include <iostream>
#include <map>
#include <random>
#include <vector>

/*
Game state is stored in a 32 bit integer:
    - the first 18 bits are the values of each position
    - row-major from top-right
    - (00 = empty, 01 = cross, 10 = nought)
    - the 19th bit is 0 for cross' turn and 1 for nought's
*/

typedef __int32_t board_t;
typedef char symbol_t;
typedef char position_t;
typedef signed char value_t;

const symbol_t empty = 0x0;
const symbol_t x = 0x1;
const symbol_t o = 0x3;
const symbol_t both = 0x4;
const board_t pos_masks[9] = {0xC0000000, 0x30000000, 0x0C000000, 0x03000000, 0x00C00000, 0x00300000, 0x000C0000, 0x00030000, 0x0000C000};
const char pos_shifts[9] = {30, 28, 26, 24, 22, 20, 18, 16, 14};
const board_t turn_mask = 0x00002000;
const char turn_shift = 13;
const char num_squares = 9;

using namespace std;

map<board_t, position_t> optimal_moves;
map<board_t, value_t> optimal_values;

board_t set_pos(board_t state, position_t pos, symbol_t symbol) {
    return state | (pos_masks[pos] & (symbol << pos_shifts[pos]));
}

board_t get_pos(board_t state, position_t pos) {
    return (state >> pos_shifts[pos]) & 0x3;
}

board_t reset_state(board_t state) {
    return 0;
}

board_t swap_turn(board_t state) {
    return state ^ turn_mask;
}

symbol_t whose_turn(board_t state) {
    return (((state >> turn_shift) & 0x1) == 0) ? x : o;
}

symbol_t other_player(symbol_t player) {
    return (player == x) ? o : x;
}

char game_over(board_t state) {
    // Check for three in a row
    // Rows
    if (get_pos(state, 0) != empty && get_pos(state, 0) == get_pos(state, 1) && get_pos(state, 1) == get_pos(state, 2))
        return get_pos(state, 0);
    if (get_pos(state, 3) != empty && get_pos(state, 3) == get_pos(state, 4) && get_pos(state, 4) == get_pos(state, 5))
        return get_pos(state, 3);
    if (get_pos(state, 6) != empty && get_pos(state, 6) == get_pos(state, 7) && get_pos(state, 7) == get_pos(state, 8))
        return get_pos(state, 6);
    // Columns
    if (get_pos(state, 0) != empty && get_pos(state, 0) == get_pos(state, 3) && get_pos(state, 3) == get_pos(state, 6))
        return get_pos(state, 0);
    if (get_pos(state, 1) != empty && get_pos(state, 1) == get_pos(state, 4) && get_pos(state, 4) == get_pos(state, 7))
        return get_pos(state, 1);
    if (get_pos(state, 2) != empty && get_pos(state, 2) == get_pos(state, 5) && get_pos(state, 5) == get_pos(state, 8))
        return get_pos(state, 2);
    // Diagonals
    if (get_pos(state, 0) != empty && get_pos(state, 0) == get_pos(state, 4) && get_pos(state, 4) == get_pos(state, 8))
        return get_pos(state, 0);
    if (get_pos(state, 2) != empty && get_pos(state, 2) == get_pos(state, 4) && get_pos(state, 4) == get_pos(state, 6))
        return get_pos(state, 2);
    
    // Check for draw
    // TODO: update to check if no-one can win, even if grid isn't full
    char sum = 0;
    for (position_t pos = 0; pos < num_squares; ++pos)
        sum += get_pos(state, pos) != empty ? 1 : 0;
    if (sum == num_squares)
        return both;
    
    return 0;
}

board_t take_optimal_turn(board_t state) {
    return set_pos(state, optimal_moves[state], whose_turn(state));
}

board_t take_random_turn(board_t state) {
    vector<position_t> empty;
    for (position_t pos = 0; pos < num_squares; ++pos)
        if (get_pos(state, pos) == empty)
            empty.push_back(pos);
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, empty.size());
    return set_pos(state, dis(gen), whose_turn(state));
}

void enumerate_moves(board_t state) {
    if (optimal_moves.count(state) == 1)
        return;
    
    char game_state = game_over(state);
    if (game_state != 0) {
        optimal_moves[state] = -1;
        if (game_state == both)
            optimal_values[state] = 0;
        else if (game_state == whose_turn(state))
            optimal_values[state] = 1;
        else
            optimal_values[state] = -1;
        return;
    }
    
    position_t best_pos;
    value_t best_value = -2;
    for (position_t pos = 0; pos < num_squares; ++pos) {
        if (get_pos(state, pos) == empty) {
            board_t new_state = set_pos(state, pos, whose_turn(state));
            new_state = swap_turn(new_state);
            enumerate_moves(new_state);
            
            value_t value = -optimal_values[new_state];
            if (value > best_value) {
                best_value = value;
                best_pos = pos;
            }
        }
    }
    optimal_values[state] = best_value;
    optimal_moves[state] = best_pos;
}

void print_state(board_t state) {
    for (position_t pos = 0; pos < num_squares; ++pos) {
        char symbol = ' ';
        char at_pos = get_pos(state, pos);
        if (at_pos == x)
            symbol = 'X';
        else if (at_pos == o)
            symbol = 'O';
        cout << symbol;
        if ((pos + 1) % 3 == 0)
            cout << endl;
        else
            cout << "|";
    }
    cout << endl;
}

int main() {
    enumerate_moves(0);
    
    while (1) {
        board_t state;
        state = reset_state(state);
        while (game_over(state) == 0) {
            state = take_random_turn(state);
            state = swap_turn(state);
            //print_state(state);
        }
        if (game_over(state) == x)
            cout << "Crosses won!" << endl;
        else if (game_over(state) == o)
            cout << "Noughts won!" << endl;
        else if (game_over(state) == both)
            cout << "Draw!" << endl;
    }
    
    return 0;
}
