cat > connect4.c <<'EOF'
#include <stdio.h>
#include <stdlib.h>

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

int drop_piece(int col, char token) {
    if (col < 0 || col >= COLS) return -1;
    int r = 0;
    while (r < ROWS) {
        if (board[r][col] == '.') { board[r][col] = token; return r; }
        r++;
    }
    return -1;
}

int count_dir(int r, int c, int dr, int dc, char token) {
    int cnt = 0;
    while (r >= 0 && r < ROWS && c >= 0 && c < COLS && board[r][c] == token) {
        cnt++; r += dr; c += dc;
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

int main() {
    clear_board();
    char player = 'A';
    while (1) {
        print_board();
        printf("Player %c - choose column (1-7): ", player);
        int col = 0;
        if (scanf("%d", &col) != 1) { puts("\nInvalid input. Exiting."); return 0; }
        col -= 1;
        int row = drop_piece(col, player);
        if (row == -1) { puts("Invalid move (column full or out of range). Try again."); continue; }
        if (is_winning_move(row, col, player)) { print_board(); printf("Player %c WINS!\n", player); break; }
        if (is_draw()) { print_board(); puts("DRAW!"); break; }
        player = (player == 'A') ? 'B' : 'A';
    }
    return 0;
}
EOF
