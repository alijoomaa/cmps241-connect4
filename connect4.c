#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ROWS 6
#define COLS 7

char board[ROWS][COLS];

void clear_board() {
    int r = 0;
    while (r < ROWS) {
        int c = 0;
        while (c < COLS) { board[r][c] = '.'; c++; }
        r++;
    }
}

void print_board() {
    puts("\n  1 2 3 4 5 6 7");
    int r = ROWS - 1;
    while (r >= 0) {
        printf("%d ", r + 1);
        int c = 0;
        while (c < COLS) { printf("%c ", board[r][c]); c++; }
        puts("");
        r--;
    }
}

int is_column_full(int col) {
    if (col < 0 || col >= COLS) return 1;
    return board[ROWS - 1][col] != '.';
}

int drop_piece(int col, char token) {
    if (col < 0 || col >= COLS) return -1;
    int r = 0;
    while (r < ROWS) {
        if (board[r][col] == '.') {
            board[r][col] = token;
            return r;
        }
        r++;
    }
    return -1;
}

int count_dir(int r, int c, int dr, int dc, char token) {
    int cnt = 0;
    while (r >= 0 && r < ROWS && c >= 0 && c < COLS && board[r][c] == token) {
        cnt++;
        r += dr;
        c += dc;
    }
    return cnt;
}

int is_winning_move(int last_r, int last_c, char token) {
    int dirs[4][2] = {{0,1},{1,0},{1,1},{1,-1}};
    int i = 0;
    while (i < 4) {
        int dr = dirs[i][0], dc = dirs[i][1];
        int total = count_dir(last_r, last_c,  dr,  dc, token)
                  + count_dir(last_r, last_c, -dr, -dc, token) - 1;
        if (total >= 4) return 1;
        i++;
    }
    return 0;
}

int is_draw() {
    int c = 0;
    while (c < COLS) {
        if (board[ROWS - 1][c] == '.') return 0;
        c++;
    }
    return 1;
}

int any_valid_moves() {
    int c = 0;
    while (c < COLS) {
        if (!is_column_full(c)) return 1;
        c++;
    }
    return 0;
}

int bot_choose_column_easy() {
    int valid_cols[COLS];
    int n = 0;
    int c = 0;
    while (c < COLS) {
        if (!is_column_full(c)) {
            valid_cols[n] = c;
            n++;
        }
        c++;
    }
    if (n == 0) return -1;
    int idx = rand() % n;
    return valid_cols[idx];
}

int find_winning_move_for(char token) {
    int col = 0;
    while (col < COLS) {
        if (!is_column_full(col)) {
            int row = drop_piece(col, token);
            if (row != -1) {
                int win = is_winning_move(row, col, token);
                board[row][col] = '.';
                if (win) return col;
            }
        }
        col++;
    }
    return -1;
}

int bot_choose_column_medium() {
    int win_col = find_winning_move_for('B');
    if (win_col != -1) return win_col;
    int block_col = find_winning_move_for('A');
    if (block_col != -1) return block_col;
    int priority[COLS] = {3, 2, 4, 1, 5, 0, 6};
    int i = 0;
    while (i < COLS) {
        int col = priority[i];
        if (!is_column_full(col)) return col;
        i++;
    }
    return bot_choose_column_easy();
}

int bot_choose_column(int difficulty) {
    if (difficulty == 1) return bot_choose_column_easy();
    if (difficulty == 2) return bot_choose_column_medium();
    return bot_choose_column_medium();
}

int main() {
    clear_board();
    srand((unsigned)time(NULL));
    int mode = 0;
    int difficulty = 1;
    puts("Select mode:");
    puts("1. Player vs Player");
    puts("2. Player vs Bot");
    printf("> ");
    if (scanf("%d", &mode) != 1 || (mode != 1 && mode != 2)) {
        puts("\nInvalid selection. Exiting.");
        return 0;
    }
    if (mode == 2) {
        puts("\nSelect bot difficulty:");
        puts("1. Easy  (random valid moves)");
        puts("2. Medium (win/block + center)");
        puts("3. Hard   (future)");
        printf("> ");
        if (scanf("%d", &difficulty) != 1 || difficulty < 1 || difficulty > 3) {
            puts("\nInvalid difficulty. Defaulting to Easy.");
            difficulty = 1;
        }
    }
    char player = 'A';
    while (1) {
        print_board();
        if (is_draw() || !any_valid_moves()) {
            puts("DRAW!");
            break;
        }
        if (mode == 2 && player == 'B') {
            int bot_col = bot_choose_column(difficulty);
            if (bot_col == -1) {
                print_board();
                puts("DRAW!");
                break;
            }
            printf("Bot (B) chooses column: %d\n", bot_col + 1);
            int row = drop_piece(bot_col, 'B');
            if (row == -1) {
                puts("Bot attempted invalid move. Forfeiting turn.");
            } else {
                if (is_winning_move(row, bot_col, 'B')) {
                    print_board();
                    puts("Player B (Bot) WINS!");
                    break;
                }
            }
        } else {
            printf("Player %c - choose column (1-7): ", player);
            int col = 0;
            if (scanf("%d", &col) != 1) {
                puts("\nInvalid input. Exiting.");
                return 0;
            }
            col -= 1;
            int row = drop_piece(col, player);
            if (row == -1) {
                puts("Invalid move (column full or out of range). Try again.");
                continue;
            }
            if (is_winning_move(row, col, player)) {
                print_board();
                printf("Player %c WINS!\n", player);
                break;
            }
        }
        if (is_draw()) {
            print_board();
            puts("DRAW!");
            break;
        }
        player = (player == 'A') ? 'B' : 'A';
    }
    return 0;
}
