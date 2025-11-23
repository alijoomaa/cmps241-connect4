#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>   
#include <pthread.h>  

#define ROWS 6
#define COLS 7

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

char board[ROWS][COLS];

double now_ms();               
double g_move_start_ms = 0.0;  
double g_time_limit_ms = 15000.0; 
int    g_time_over     = 0;    


void clear_board() {
    int r = 0;
    while (r < ROWS) {
        int c = 0;
        while (c < COLS) { board[r][c] = '.'; c++; }
        r++;
    }
}

const char* color_piece(char p) {
    if (p == 'A') return GREEN "A" RESET;
    if (p == 'B') return RED "B" RESET;
    return YELLOW "." RESET;
}

void print_board() {
    printf("\n\n");
    printf(BOLD CYAN "        CONNECT 4\n" RESET);
    printf(BOLD "    1  2  3  4  5  6  7\n" RESET);
    printf("    -------------------------\n");

    for (int r = ROWS - 1; r >= 0; r--) {
        printf(BOLD "%d | " RESET, r + 1);
        for (int c = 0; c < COLS; c++) {
            printf("%s  ", color_piece(board[r][c]));
        }
        printf("\n");
    }
    printf("    -------------------------\n\n");
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


#define TT_SIZE 16777259  
#define TT_INVALID 0
#define TT_EXACT 1
#define TT_LOWER 2
#define TT_UPPER 3

typedef struct {
    unsigned long long hash;
    int score;
    int depth;
    int flag;
    int best_move;
} TTEntry;

TTEntry transposition_table[TT_SIZE];
int tt_initialized = 0;

typedef struct {
    unsigned long long botBits;
    unsigned long long humanBits;
    unsigned long long mask;
} BitboardState;

void init_transposition_table() {
    if (tt_initialized) return;
    int i = 0;
    while (i < TT_SIZE) {
        transposition_table[i].hash = 0;
        transposition_table[i].score = 0;
        transposition_table[i].depth = -1;
        transposition_table[i].flag = TT_INVALID;
        transposition_table[i].best_move = -1;
        i++;
    }
    tt_initialized = 1;
}

BitboardState from_board() {
    BitboardState state;
    state.botBits = 0;
    state.humanBits = 0;
    state.mask = 0;

    unsigned long long bit = 1;
    int r = 0;
    while (r < ROWS) {
        int c = 0;
        while (c < COLS) {
            if (board[r][c] == 'B') {
                state.botBits |= bit;
                state.mask |= bit;
            } else if (board[r][c] == 'A') {
                state.humanBits |= bit;
                state.mask |= bit;
            }
            bit <<= 1;
            c++;
        }
        r++;
    }
    return state;
}

int bitboard_is_win(char token) {
    int r = 0;
    while (r < ROWS) {
        int c = 0;
        while (c < COLS) {
            if (board[r][c] == token) {
                if (is_winning_move(r, c, token)) return 1;
            }
            c++;
        }
        r++;
    }
    return 0;
}

int game_result_for_bot() {
    if (bitboard_is_win('B')) return 1;
    if (bitboard_is_win('A')) return -1;
    if (is_draw()) return 0;
    return 2;
}

unsigned long long hash_board() {
    BitboardState state = from_board();
    unsigned long long hash = state.botBits;
    hash ^= state.humanBits * 0x9e3779b9;
    hash ^= state.mask * 0x517cc1b7;
    return hash;
}

int tt_lookup(unsigned long long hash, int depth, int alpha, int beta, int *best_move) {
    int index = (int)(hash % TT_SIZE);
    if (index < 0) index += TT_SIZE;
    TTEntry *entry = &transposition_table[index];

    if (entry->hash == hash && entry->depth >= depth && entry->flag != TT_INVALID) {
        if (best_move) *best_move = entry->best_move;
        if (entry->flag == TT_EXACT) return entry->score;
        if (entry->flag == TT_LOWER && entry->score >= beta) return entry->score;
        if (entry->flag == TT_UPPER && entry->score <= alpha) return entry->score;
    }
    return 99999999; 
}

void tt_store(unsigned long long hash, int depth, int score, int flag, int best_move) {
    int index = (int)(hash % TT_SIZE);
    if (index < 0) index += TT_SIZE;
    TTEntry *entry = &transposition_table[index];

    if (entry->flag == TT_INVALID ||
        (entry->hash == hash && entry->depth < depth) ||
        (entry->hash == hash && entry->depth == depth && flag == TT_EXACT && entry->flag != TT_EXACT)) {
        entry->hash = hash;
        entry->score = score;
        entry->depth = depth;
        entry->flag = flag;
        entry->best_move = best_move;
    }
}

int get_next_open_row(int col) {
    if (col < 0 || col >= COLS) return -1;
    int r = 0;
    while (r < ROWS) {
        if (board[r][col] == '.') return r;
        r++;
    }
    return -1;
}

void undo_piece(int col) {
    if (col < 0 || col >= COLS) return;
    int r = ROWS - 1;
    while (r >= 0) {
        if (board[r][col] != '.') {
            board[r][col] = '.';
            return;
        }
        r--;
    }
}


int evaluate_for_bot() {
    int score = 0;
    int r, c;
    int bot_threats = 0, human_threats = 0;
    int bot_open_3 = 0, human_open_3 = 0;
    int bot_forks = 0, human_forks = 0;

    r = 0;
    while (r < ROWS) {
        if (board[r][3] == 'B') score += 20;
        else if (board[r][3] == 'A') score -= 20;
        r++;
    }

    r = 0;
    while (r < ROWS) {
        if (board[r][2] == 'B') score += 8;
        else if (board[r][2] == 'A') score -= 8;
        if (board[r][4] == 'B') score += 8;
        else if (board[r][4] == 'A') score -= 8;
        r++;
    }

    int bot_win_count = 0, human_win_count = 0;
    int col = 0;
    while (col < COLS) {
        if (!is_column_full(col)) {
            int row = drop_piece(col, 'B');
            if (row != -1) {
                if (is_winning_move(row, col, 'B')) bot_win_count++;
                undo_piece(col);
            }
            row = drop_piece(col, 'A');
            if (row != -1) {
                if (is_winning_move(row, col, 'A')) human_win_count++;
                undo_piece(col);
            }
        }
        col++;
    }

    if (bot_win_count >= 2) {
        score += 50000;
        bot_forks++;
    }
    if (human_win_count >= 2) {
        score -= 50000;
        human_forks++;
    }

    r = 0;
    while (r < ROWS) {
        c = 0;
        while (c <= COLS - 4) {
            int bot_count = 0, human_count = 0, empty = 0;
            int i = 0;
            while (i < 4) {
                if (board[r][c + i] == 'B') bot_count++;
                else if (board[r][c + i] == 'A') human_count++;
                else empty++;
                i++;
            }

            int left_open = (c > 0 && board[r][c - 1] == '.');
            int right_open = (c + 4 < COLS && board[r][c + 4] == '.');
            int both_open = left_open && right_open;
            int is_open = left_open || right_open;

            if (bot_count == 4) score += 100000;
            else if (bot_count == 3 && empty == 1) {
                if (both_open) { score += 1200; bot_open_3++; }
                else if (is_open) { score += 400; bot_threats++; }
                else score += 200;
            }
            else if (bot_count == 2 && empty == 2) {
                if (both_open) score += 40;
                else if (is_open) score += 20;
                else score += 10;
            }

            if (human_count == 4) score -= 100000;
            else if (human_count == 3 && empty == 1) {
                if (both_open) { score -= 3000; human_open_3++; }
                else if (is_open) { score -= 1500; human_threats++; }
                else score -= 300;
            }
            else if (human_count == 2 && empty == 2) {
                if (both_open) score -= 40;
                else if (is_open) score -= 20;
                else score -= 10;
            }
            c++;
        }
        r++;
    }

    r = 0;
    while (r <= ROWS - 4) {
        c = 0;
        while (c < COLS) {
            int bot_count = 0, human_count = 0, empty = 0;
            int i = 0;
            while (i < 4) {
                if (board[r + i][c] == 'B') bot_count++;
                else if (board[r + i][c] == 'A') human_count++;
                else empty++;
                i++;
            }

            if (bot_count == 4) score += 100000;
            else if (bot_count == 3 && empty == 1) {
                score += 600;
                bot_threats++;
            }
            else if (bot_count == 2 && empty == 2) score += 25;

            if (human_count == 4) score -= 100000;
            else if (human_count == 3 && empty == 1) {
                score -= 1800;
                human_threats++;
            }
            else if (human_count == 2 && empty == 2) {
                score -= 25;
            }
            c++;
        }
        r++;
    }

    r = 0;
    while (r <= ROWS - 4) {
        c = 0;
        while (c <= COLS - 4) {
            int bot_count = 0, human_count = 0, empty = 0;
            int i = 0;
            while (i < 4) {
                if (board[r + i][c + i] == 'B') bot_count++;
                else if (board[r + i][c + i] == 'A') human_count++;
                else empty++;
                i++;
            }

            int left_open = (r > 0 && c > 0 && board[r - 1][c - 1] == '.');
            int right_open = (r + 4 < ROWS && c + 4 < COLS && board[r + 4][c + 4] == '.');
            int both_open = left_open && right_open;
            int is_open = left_open || right_open;

            if (bot_count == 4) score += 100000;
            else if (bot_count == 3 && empty == 1) {
                if (both_open) { score += 1200; bot_open_3++; }
                else if (is_open) { score += 400; bot_threats++; }
                else score += 200;
            }
            else if (bot_count == 2 && empty == 2) {
                if (both_open) score += 40;
                else if (is_open) score += 20;
                else score += 10;
            }

            if (human_count == 4) score -= 100000;
            else if (human_count == 3 && empty == 1) {
                if (both_open) { score -= 3000; human_open_3++; }
                else if (is_open) { score -= 1500; human_threats++; }
                else score -= 300;
            }
            else if (human_count == 2 && empty == 2) {
                if (both_open) score -= 40;
                else if (is_open) score -= 20;
                else score -= 10;
            }
            c++;
        }
        r++;
    }

    r = 0;
    while (r <= ROWS - 4) {
        c = 3;
        while (c < COLS) {
            int bot_count = 0, human_count = 0, empty = 0;
            int i = 0;
            while (i < 4) {
                if (board[r + i][c - i] == 'B') bot_count++;
                else if (board[r + i][c - i] == 'A') human_count++;
                else empty++;
                i++;
            }

            int left_open = (r > 0 && c + 1 < COLS && board[r - 1][c + 1] == '.');
            int right_open = (r + 4 < ROWS && c - 4 >= 0 && board[r + 4][c - 4] == '.');
            int both_open = left_open && right_open;
            int is_open = left_open || right_open;

            if (bot_count == 4) score += 100000;
            else if (bot_count == 3 && empty == 1) {
                if (both_open) { score += 1200; bot_open_3++; }
                else if (is_open) { score += 400; bot_threats++; }
                else score += 200;
            }
            else if (bot_count == 2 && empty == 2) {
                if (both_open) score += 40;
                else if (is_open) score += 20;
                else score += 10;
            }

            if (human_count == 4) score -= 100000;
            else if (human_count == 3 && empty == 1) {
                if (both_open) { score -= 3000; human_open_3++; }
                else if (is_open) { score -= 1500; human_threats++; }
                else score -= 300;
            }
            else if (human_count == 2 && empty == 2) {
                if (both_open) score -= 40;
                else if (is_open) score -= 20;
                else score -= 10;
            }
            c++;
        }
        r++;
    }

    if (bot_threats >= 2) score += 2500;
    if (bot_open_3 >= 2) score += 5000;
    if (bot_forks > 0) score += 10000;

    if (human_threats >= 2) score -= 5000;
    if (human_open_3 >= 2) score -= 8000;
    if (human_forks > 0) score -= 20000;

    return score;
}




typedef struct {
    char board_copy[ROWS][COLS];
    int partial_score;   
    int thread_id;       
} ThreadEvalTask;

int eval_lightweight(char b[ROWS][COLS]) {
    int score = 0;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (b[r][c] == 'B') score++;
            else if (b[r][c] == 'A') score--;
        }
    }
    return score;
}

void* thread_eval_func(void *arg) {
    ThreadEvalTask *task = (ThreadEvalTask*)arg;

    task->partial_score = eval_lightweight(task->board_copy);

    
    return NULL;
}


void run_multithreaded_helper() {
    ThreadEvalTask T1, T2;

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            T1.board_copy[r][c] = board[r][c];
            T2.board_copy[r][c] = board[r][c];
        }
    }

    T1.thread_id = 1;
    T2.thread_id = 2;

    pthread_t th1, th2;

    pthread_create(&th1, NULL, thread_eval_func, &T1);
    pthread_create(&th2, NULL, thread_eval_func, &T2);

    pthread_join(th1, NULL);
    pthread_join(th2, NULL);

    int combined_score = T1.partial_score + T2.partial_score;

   
}





int opening_book_move_for_bot_small() {
    int a_count = 0, b_count = 0, empty_count = 0;
    int r, c;

    r = 0;
    while (r < ROWS) {
        c = 0;
        while (c < COLS) {
            if (board[r][c] == 'A') a_count++;
            else if (board[r][c] == 'B') b_count++;
            else empty_count++;
            c++;
        }
        r++;
    }

    int pieces = a_count + b_count;
    if (pieces > 3) return -1;  

    if (pieces > 0 && (b_count > a_count + 1 || a_count > b_count + 1)) {
        return -1;
    }

    char bottom_owner[COLS];
    for (c = 0; c < COLS; c++) {
        bottom_owner[c] = '.';
        r = 0;
        while (r < ROWS) {
            if (board[r][c] != '.') {
                bottom_owner[c] = board[r][c];
                break;
            }
            r++;
        }
    }

    int priority[COLS] = {3, 2, 4, 1, 5, 0, 6};
    int i;

    
    if (pieces == 0) {
        if (!is_column_full(3)) return 3;
        for (i = 0; i < COLS; i++) {
            if (!is_column_full(priority[i])) return priority[i];
        }
        return -1;
    }

    
    if (pieces == 1) {
        int colA = -1;
        for (c = 0; c < COLS; c++) {
            if (bottom_owner[c] == 'A') {
                colA = c;
                break;
            }
        }
        if (colA == -1) return -1;

        if (colA == 3) {
            if (!is_column_full(2)) return 2;
            if (!is_column_full(4)) return 4;
        } else {
            if (!is_column_full(3)) return 3;
        }

        for (i = 0; i < COLS; i++) {
            if (!is_column_full(priority[i])) return priority[i];
        }
        return -1;
    }

    
    if (pieces == 2) {
        int colB = -1, colA = -1;
        for (c = 0; c < COLS; c++) {
            if (bottom_owner[c] == 'B') colB = c;
            else if (bottom_owner[c] == 'A') colA = c;
        }

        if (colB == 3) {
            if (colA == 2) {
                if (!is_column_full(4)) return 4;
            } else if (colA == 4) {
                if (!is_column_full(2)) return 2;
            } else {
                if (!is_column_full(2)) return 2;
                if (!is_column_full(4)) return 4;
            }
        } else {
            if (!is_column_full(3)) return 3;
        }

        for (i = 0; i < COLS; i++) {
            if (!is_column_full(priority[i])) return priority[i];
        }
        return -1;
    }

    
    if (pieces == 3) {
        if (bottom_owner[3] == '.' && !is_column_full(3)) {
            return 3;
        }
        int priority2[COLS] = {3, 2, 4, 1, 5, 0, 6};
        for (i = 0; i < COLS; i++) {
            if (!is_column_full(priority2[i])) return priority2[i];
        }
        return -1;
    }

    return -1;
}



typedef struct {
    uint64_t hash;
    int8_t   best_col;
    int8_t   outcome;
    int8_t   depth;
    int8_t   reserved;
    int32_t  padding;
} BookEntry;

BookEntry *g_opening_book = NULL;
int        g_opening_book_size = 0;
int        g_opening_book_loaded = 0;


int load_opening_book(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Could not open opening book file: %s\n", filename);
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (filesize <= 0 || (filesize % (long)sizeof(BookEntry)) != 0) {
        fprintf(stderr, "Opening book file has invalid size.\n");
        fclose(f);
        return 0;
    }

    g_opening_book_size = (int)(filesize / (long)sizeof(BookEntry));
    g_opening_book = (BookEntry*)malloc(sizeof(BookEntry) * g_opening_book_size);
    if (!g_opening_book) {
        fprintf(stderr, "Memory allocation failed for opening book.\n");
        fclose(f);
        g_opening_book_size = 0;
        return 0;
    }

    size_t read_count = fread(g_opening_book, sizeof(BookEntry), g_opening_book_size, f);
    fclose(f);

    if ((int)read_count != g_opening_book_size) {
        fprintf(stderr, "Failed to read opening book entries.\n");
        free(g_opening_book);
        g_opening_book = NULL;
        g_opening_book_size = 0;
        return 0;
    }

    g_opening_book_loaded = 1;
    fprintf(stderr, "Loaded opening book: %d entries from %s\n",
            g_opening_book_size, filename);
    return 1;
}



int opening_book_move_for_bot_full(int *best_col, int max_book_plies) {
    if (!g_opening_book_loaded) return 0;

    
    int a_count = 0, b_count = 0;
    int r, c;
    r = 0;
    while (r < ROWS) {
        c = 0;
        while (c < COLS) {
            if (board[r][c] == 'A') a_count++;
            else if (board[r][c] == 'B') b_count++;
            c++;
        }
        r++;
    }
    int pieces = a_count + b_count;
    if (pieces > max_book_plies) return 0;

    uint64_t h = hash_board();

    
    for (int i = 0; i < g_opening_book_size; i++) {
        if (g_opening_book[i].hash == h) {
            int col = g_opening_book[i].best_col;
            if (col >= 0 && col < COLS && !is_column_full(col)) {
                if (best_col) *best_col = col;
                return 1;
            }
            break;
        }
    }
    return 0;
}



int negamax(int alpha, int beta, char current_player, int depth, int *bestCol, int is_root, int max_depth) {
    
    if (g_time_limit_ms > 0.0) {
        double now = now_ms();
        if (now - g_move_start_ms > g_time_limit_ms) {
            g_time_over = 1;
            int eval = evaluate_for_bot();
            return (current_player == 'B') ? eval : -eval;
        }
    }

    unsigned long long hash = hash_board();
    char opponent = (current_player == 'B') ? 'A' : 'B';
    int move_order[COLS] = {3, 2, 4, 1, 5, 0, 6};
    (void)max_depth; 

    int game_result = game_result_for_bot();
    if (game_result == 1) {
        int score = (current_player == 'B') ? (1000000 + depth) : (-1000000 - depth);
        tt_store(hash, depth, score, TT_EXACT, -1);
        return score;
    } else if (game_result == -1) {
        int score = (current_player == 'B') ? (-1000000 - depth) : (1000000 + depth);
        tt_store(hash, depth, score, TT_EXACT, -1);
        return score;
    } else if (game_result == 0) {
        tt_store(hash, depth, 0, TT_EXACT, -1);
        return 0;
    }

    int tt_move = -1;
    int tt_score = tt_lookup(hash, depth, alpha, beta, &tt_move);
    if (tt_score != 99999999) {
        if (bestCol && is_root && tt_move >= 0) *bestCol = tt_move;
        return tt_score;
    }

    if (depth <= 0) {
        int eval = evaluate_for_bot();
        int score = (current_player == 'B') ? eval : -eval;
        tt_store(hash, depth, score, TT_EXACT, -1);
        return score;
    }

    
    int i = 0;
    while (i < COLS) {
        int col = move_order[i];
        if (!is_column_full(col)) {
            int row = drop_piece(col, current_player);
            if (row != -1) {
                if (is_winning_move(row, col, current_player)) {
                    undo_piece(col);
                    int score = 1000000 + depth;
                    tt_store(hash, depth, score, TT_EXACT, col);
                    if (bestCol && is_root) *bestCol = col;
                    return score;
                }
                undo_piece(col);
            }
        }
        i++;
    }

    
    int opponent_win_col = -1;
    i = 0;
    while (i < COLS) {
        int col = move_order[i];
        if (!is_column_full(col)) {
            int row = drop_piece(col, opponent);
            if (row != -1) {
                if (is_winning_move(row, col, opponent)) {
                    undo_piece(col);
                    opponent_win_col = col;
                    break;
                }
                undo_piece(col);
            }
        }
        i++;
    }

    if (opponent_win_col != -1) {
        if (!is_column_full(opponent_win_col)) {
            int row = drop_piece(opponent_win_col, current_player);
            if (row != -1) {
                if (is_winning_move(row, opponent_win_col, current_player)) {
                    undo_piece(opponent_win_col);
                    int score = 1000000 + depth;
                    tt_store(hash, depth, score, TT_EXACT, opponent_win_col);
                    if (bestCol && is_root) *bestCol = opponent_win_col;
                    return score;
                }
                int score = -negamax(-beta, -alpha, opponent, depth - 1, NULL, 0, max_depth);
                undo_piece(opponent_win_col);
                tt_store(hash, depth, score, TT_EXACT, opponent_win_col);
                if (bestCol && is_root) *bestCol = opponent_win_col;
                return score;
            }
        }
        int score = -1000000 - depth;
        tt_store(hash, depth, score, TT_EXACT, -1);
        return score;
    }

    int valid_moves[COLS];
    int valid_count = 0;

    if (tt_move >= 0 && tt_move < COLS && !is_column_full(tt_move)) {
        valid_moves[valid_count] = tt_move;
        valid_count++;
    }

    i = 0;
    while (i < COLS) {
        if (move_order[i] != tt_move && !is_column_full(move_order[i])) {
            valid_moves[valid_count] = move_order[i];
            valid_count++;
        }
        i++;
    }

    if (valid_count == 0) {
        tt_store(hash, depth, 0, TT_EXACT, -1);
        return 0;
    }

    int best_score = -2000000;
    int best_move = -1;
    int flag = TT_UPPER;
    i = 0;
    while (i < valid_count) {
        int col = valid_moves[i];
        int row = drop_piece(col, current_player);
        if (row == -1) {
            i++;
            continue;
        }

        int score = -negamax(-beta, -alpha, opponent, depth - 1, NULL, 0, max_depth);
        undo_piece(col);

        if (score > best_score) {
            best_score = score;
            best_move = col;
            if (bestCol && is_root) *bestCol = col;
        }

        if (score > alpha) {
            alpha = score;
            flag = TT_EXACT;
        }
        if (alpha >= beta) {
            tt_store(hash, depth, alpha, TT_LOWER, col);
            return alpha;
        }
        i++;
    }

    tt_store(hash, depth, best_score, flag, best_move);
    return best_score;
}



int bot_choose_column_hard() {
    init_transposition_table();

    
    g_move_start_ms = now_ms();
    g_time_limit_ms = 15000.0; 
    g_time_over     = 0;

    
    
    
    run_multithreaded_helper();

    
    int book_col_full = -1;
    if (opening_book_move_for_bot_full(&book_col_full, 24)) {
        if (!is_column_full(book_col_full)) {
            return book_col_full;
        }
    }

    
    int ob_small = opening_book_move_for_bot_small();
    if (ob_small != -1 && !is_column_full(ob_small)) {
        return ob_small;
    }

    
    int win_move = find_winning_move_for('B');
    if (win_move != -1) return win_move;

    int block_move = find_winning_move_for('A');
    if (block_move != -1) return block_move;

    
    int empty_count = 0;
    int c = 0;
    while (c < COLS) {
        int r = 0;
        while (r < ROWS) {
            if (board[r][c] == '.') empty_count++;
            r++;
        }
        c++;
    }

    int start_depth = 8;
    int max_depth;

    if (empty_count <= 10) {
        max_depth = empty_count;
    } else if (empty_count <= 20) {
        max_depth = 14;
    } else {
        max_depth = 13;
    }

    int best_col = -1;
    int best_score = -2000000;
    int depth = start_depth;

    
    while (depth <= max_depth) {
        int current_best = -1;
        int current_score = negamax(-2000000, 2000000, 'B', depth, &current_best, 1, max_depth);

        if (g_time_over) {
            break;
        }

        if (current_best >= 0 && current_best < COLS && !is_column_full(current_best)) {
            best_col = current_best;
            best_score = current_score;

            if (current_score >= 1000000 || current_score <= -1000000) {
                break;
            }
        }

        depth++;

        if (depth > 12 && (best_score > 800000 || best_score < -800000)) {
            break;
        }
    }

    if (best_col >= 0 && best_col < COLS && !is_column_full(best_col)) {
        return best_col;
    }

    
    int move_order[COLS] = {3, 2, 4, 1, 5, 0, 6};
    int best_score_fallback = -2000000;
    int best_move_fallback = -1;
    int i = 0;
    int fallback_depth = (max_depth >= 11) ? 11 : max_depth;

    while (i < COLS) {
        int col2 = move_order[i];
        if (is_column_full(col2)) {
            i++;
            continue;
        }

        int row = drop_piece(col2, 'B');
        if (row == -1) {
            i++;
            continue;
        }

        if (is_winning_move(row, col2, 'B')) {
            undo_piece(col2);
            return col2;
        }

        int eval = -negamax(-2000000, 2000000, 'A', fallback_depth - 1, NULL, 0, max_depth);
        undo_piece(col2);

        if (eval > best_score_fallback) {
            best_score_fallback = eval;
            best_move_fallback = col2;
        }
        i++;
    }

    if (best_move_fallback >= 0 && best_move_fallback < COLS && !is_column_full(best_move_fallback)) {
        return best_move_fallback;
    }

    i = 0;
    while (i < COLS) {
        int col3 = move_order[i];
        if (!is_column_full(col3)) return col3;
        i++;
    }

    return -1;
}



int bot_choose_column(int difficulty) {
    if (difficulty == 1) return bot_choose_column_easy();
    if (difficulty == 2) return bot_choose_column_medium();
    if (difficulty == 3) return bot_choose_column_hard();
    return bot_choose_column_medium();
}



double now_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}



int main() {
    clear_board();
    srand((unsigned)time(NULL));

    
    
    load_opening_book("c4_book_12ply.dat");

    int mode, difficulty = 2, starter = 1;
    printf(CYAN BOLD "\nSelect mode:\n" RESET);
    printf("1. Player vs Player\n");
    printf("2. Player vs Bot\n> ");
    scanf("%d", &mode);

    if (mode == 2) {
        printf(CYAN BOLD "\nSelect bot difficulty:\n" RESET);
        printf("1. Easy\n2. Medium\n3. Hard\n> ");
        scanf("%d", &difficulty);

        printf(CYAN BOLD "\nWho starts first?\n" RESET);
        printf("1. Player (A)\n2. Bot (B)\n> ");
        scanf("%d", &starter);
    }

    char player = (mode == 1 ? 'A' : (starter == 2 ? 'B' : 'A'));

    while (1) {
        print_board();

        if (is_draw()) { printf("DRAW!\n"); break; }

        if (mode == 2 && player == 'B') {
            printf(YELLOW BOLD "\nBot is thinking...\n" RESET);

            double t1 = now_ms();
            int col = bot_choose_column(difficulty);
            double t2 = now_ms();
            double elapsed = (t2 - t1) / 1000.0;

            printf(RED BOLD "Bot (B) plays column: %d\n" RESET, col + 1);
            printf(CYAN BOLD "Time taken: %.3f seconds\n\n" RESET, elapsed);

            int r = drop_piece(col, 'B');
            if (is_winning_move(r, col, 'B')) {
                print_board();
                printf(RED BOLD "Bot WINS!\n" RESET);
                break;
            }
        }
        else {
            int col;
            printf(GREEN BOLD "Player %c, choose column (1-7): " RESET, player);
            scanf("%d", &col);
            col--;

            int r = drop_piece(col, player);
            if (r == -1) {
                printf(RED "Invalid move! Try again.\n" RESET);
                continue;
            }
            if (is_winning_move(r, col, player)) {
                print_board();
                printf(GREEN BOLD "Player %c WINS!\n" RESET, player);
                break;
            }
        }

        if (is_draw()) {
            print_board();
            printf("DRAW!\n");
            break;
        }

        player = (player == 'A') ? 'B' : 'A';
    }

    if (g_opening_book) {
        free(g_opening_book);
    }

    return 0;
}

