#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>
#include <time.h>
#include <float.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <time.h>
#include "myprog.h"
#include "defs.h"
#include "playerHelper.c"

typedef struct IDSArgs
{
    int player;             // Maximizing player.
    State state;            // State input
    volatile int *bestMove; // The best-move int to write to.
    clock_t start;          // Max depth to iteratively search upto.
} IDSArgs_t;

IDSArgs_t arguments;

/* copy numbytes from src to destination
   before the copy happens the destSize bytes of the destination array is set to 0s
*/

struct timespec timer;

static inline void reset_timer(struct timespec *timer)
{
    clock_gettime(CLOCK_MONOTONIC, timer);
}

static inline int timeup()
{
    struct timespec end_time;
    reset_timer(&end_time); // Get current time.
    double elapsed_time = (double)(end_time.tv_sec - timer.tv_sec) +
                          (double)(end_time.tv_nsec - timer.tv_nsec) / 1e9;

    if (elapsed_time >= (SecPerMove - 0.05))
        return 1;
    return 0;
}

static inline double dmax(double a, double b);
static inline double dmin(double a, double b);
static inline double dabs(double a);
double evalRat(struct State *state, int maxplayer);
static inline int isExposed(char board[8][8], int x, int y, int color);

double piecediff;
double exposeddiff;
double clusterscore;
double kingsdiff;
double centercount;
double stuckcount;
double backrowmultiplier;

volatile int ids_retval = 0;
struct timespec timer;

int randomseltoggle = 0; // Random selection toggle.

static inline double dmax(double a, double b)
{
    if (a > b)
        return a;
    return b;
}

static inline double dmin(double a, double b)
{
    if (a < b)
        return a;
    return b;
}

static inline double dabs(double a)
{
    if (a < 0)
        return -a;
    return a;
}

void safeCopy(char *dest, char *src, int destSize, int numbytes)
{
    memset(dest, 0, destSize);
    memcpy(dest, src, numbytes);
}

void printBoard(struct State *state)
{
    int y, x;

    for (y = 0; y < 8; y++)
    {
        for (x = 0; x < 8; x++)
        {
            if (x % 2 != y % 2)
            {
                if (empty(state->board[y][x]))
                {
                    fprintf(stderr, " ");
                }
                else if (king(state->board[y][x]))
                {
                    if (color(state->board[y][x]) == 2)
                        fprintf(stderr, "B");
                    else
                        fprintf(stderr, "A");
                }
                else if (piece(state->board[y][x]))
                {
                    if (color(state->board[y][x]) == 2)
                        fprintf(stderr, "b");
                    else
                        fprintf(stderr, "a");
                }
            }
            else
            {
                fprintf(stderr, " ");
            }
        }
        fprintf(stderr, "\n");
    }
}

double evalSupportPieces(struct State *state, int clusterforplayer)
{
    // Evaluate the "cluster" metric for the given board state.
    int p1cluster = 0, p2cluster = 0;
    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            char ch = state->board[y][x];
            if (!empty(ch))
            {
                if (color(ch) == 1)
                {
                    int dx[2] = {1, -1};
                    int dy[2] = {-1, -1};
                    // Working with p1.
                    // Check all diagonal stuffs.
                    for (int i = 0; i < 2; i++)
                    {
                        int newX = x + dx[i];
                        int newY = y + dy[i];

                        if (newX >= 0 && newX < 8 && newY >= 0 && newY < 8)
                        {
                            if (color(state->board[newY][newX]) == 1)
                            {
                                p1cluster += 1;
                            }
                            // TODO: Penalize/reward when there's an opponent nearby.
                        }
                    }
                }
                else
                {
                    int dx[2] = {1, -1};
                    int dy[2] = {1, 1};
                    // Working with p2.
                    // Check all diagonal stuffs.
                    for (int i = 0; i < 2; i++)
                    {
                        int newX = x + dx[i];
                        int newY = y + dy[i];

                        if (newX >= 0 && newX < 8 && newY >= 0 && newY < 8)
                        {
                            if (color(state->board[newY][newX]) == 2)
                            {
                                p2cluster += 1;
                            }
                        }
                    }
                }
            }
        }
    }

    if (clusterforplayer == 1)
        return p1cluster;
    return p2cluster;
}

inline int isExposed(char board[8][8], int x, int y, int color)
{
    // Check if there are no friendly pieces diagonally adjacent to the current piece
    int dx[4] = {1, 1, -1, -1};
    int dy[4] = {1, -1, 1, -1};

    for (int i = 0; i < 4; i++)
    {
        int newX = x + dx[i];
        int newY = y + dy[i];

        if (newX >= 0 && newX < 8 && newY >= 0 && newY < 8)
        {
            if (color(board[newY][newX]) == color)
            {
                return 0;
            }
        }
    }

    return 1;
}

double evalRat(struct State *state, int maxplayer)
{
    double score = 0.0;
    double p1pieces = 0.0;
    double p2pieces = 0.0;
    double p1kings = 0.0;
    double p2kings = 0.0;
    double p1backrow = 0.0;
    double p2backrow = 0.0;
    double p1exposed = 0.0;
    double p2exposed = 0.0;
    double p1center  = 0.0;
    double p2center  = 0.0;
    double p1clusterdistance = 0.0;
    double p2clusterdistance = 0.0;

    double p1piecex[12];
    double p1piecey[12];
    double p2piecex[12];
    double p2piecey[12];

    int p1distanceidx = 0.0;
    int p2distanceidx = 0.0;

    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            if (!empty(state->board[y][x]))
            {
                char ch = state->board[y][x];
                int color = color(state->board[y][x]);
                if (color == 1){
                    p1piecex[p1distanceidx++] = x;
                    p1piecey[p1distanceidx]   = y; // WARN: DO NOT INCREMENT TWICE!
                    
                    if (y > 2 )
                        p1center += 1.0;

                    p1pieces++;
                    if(king(ch))
                        p1kings++;

                    if (y < 2)
                        p1backrow++;

                    // Check for exposed pieces.
                    // For p1, any piece can be exposed, if slots behind it are open.
                    if (y > 0 && x > 0 && x < 7)
                        if (empty(state->board[y-1][x-1]) || empty(state->board[y-1][x+1]))
                            p1exposed += 1;

                } else {
                    p2piecex[p2distanceidx++] = x;
                    p2piecey[p2distanceidx]   = y; // WARN: See warning above.

                    p2pieces++;

                    if (y < 5)
                        p2center += 1.0;

                    if(king(ch))
                        p2kings++;

                    if(y > 5)
                        p2backrow++;

                    if (y < 7 && x > 0 && x < 7)
                        if (empty(state->board[y+1][x-1]) || empty(state->board[y+1][x+1]))
                            p2exposed += 1;
                }
           }
        }
    }

    // Calculate distance.
    for (int i = 0; i < p1distanceidx; i++)
        for (int j = 0; j < p1distanceidx; j++) // (x1-x2) ^ 2 + (y1 - y2) ^ 2.
            {
                p1clusterdistance += (p1piecex[i] - p1piecex[j]) * (p1piecex[i] - p1piecex[j]);
                p1clusterdistance += (p1piecey[i] - p1piecey[j]) * (p1piecey[i] - p1piecey[j]);
            }

    for (int i = 0; i < p2distanceidx; i++)
        for (int j = 0; j < p2distanceidx; j++) // (x1-x2) ^ 2 + (y1 - y2) ^ 2.
            {
                p2clusterdistance += (p2piecex[i] - p2piecex[j]) * (p2piecex[i] - p2piecex[j]);
                p2clusterdistance += (p2piecey[i] - p2piecey[j]) * (p2piecey[i] - p2piecey[j]);
            }


    // Increase everything (prevent inf. division) (TOGGLE ON WHILE USING RATIO HEURISTICS)
    p1pieces += 1;
    p2pieces += 1;
    p1kings += 1;
    p2kings += 1;
    p1center += 1;
    p2center += 1;

    // score = (p1pieces / p2pieces) * 20.0 + (p1kings - p2kings) * 1.8;

    if (maxplayer == 1)
        return (p1pieces / p2pieces) * 20.0 + (p1center / p2center) * 10.0 + (p1kings / p2kings) * 15.0 + p1backrow * 2 - p1exposed - p1clusterdistance * 0.001;

    return (p2pieces/ p1pieces) * 20.0 + (p2center / p1center) * 10.0 + (p2kings / p1kings) * 15.0 + p2backrow * 2 - p2exposed - p2clusterdistance * 0.001;
}

double minmax_ab(struct State state, int maxplayer, int depth, double alpha, double beta)
{
    if (timeup())
        return -0.0;

    if (depth-- == 0)
    {
        return evalRat(&state, maxplayer);
    }

    double score = (state.player == maxplayer ? -DBL_MAX : DBL_MAX);

    for (int i = 0; i < state.numLegalMoves; i++)
    {
        State newState;
        memcpy(&newState, &state, sizeof(state));
        performMove(&newState, i);

        double new_score = minmax_ab(newState, maxplayer, depth, alpha, beta);
        if (state.player == maxplayer)
        {
            score = dmax(score, new_score);
            alpha = dmax(alpha, score);
        }
        else
        {
            score = dmin(score, new_score);
            beta = dmin(beta, score);
        }

        if (beta <= alpha)
        {
            break;
        }
    }

    return score;
}
void id_search();

void id_search()
{
    State state = arguments.state;
    int player = arguments.player;
    volatile int *bestmoveindex = arguments.bestMove;

    int depth = 5;
    double bestMoveScore;

    // Shuffle the numLegalMoves list.
    int *numlegalmoves = (int *)calloc(sizeof(int), state.numLegalMoves);
    for (int i = 0; i < state.numLegalMoves; i++)
    {
        numlegalmoves[i] = i;
    }
    for (int i = 0; i < state.numLegalMoves; i++)
    {
        for (int j = 0; j < state.numLegalMoves; j++)
        {
            int i1 = rand() % state.numLegalMoves, i2 = rand() % state.numLegalMoves;
            int t = numlegalmoves[i1];
            numlegalmoves[i1] = numlegalmoves[i2];
            numlegalmoves[i2] = t;
        }
    }
    // End of shuffle.

    while (depth++ < 100) // Had to put this to prevent a bug with very long "search" trees.
    {
        if (timeup())
            return;

        int bestmovesarr[100];
        int bestmovescount = 0;
        bestMoveScore = -DBL_MAX;
        int bestmoveatdepth = 0;

        fprintf(stderr, "Reached depth=%d;\n", depth);
        fprintf(stderr, "Game eval on default: %f\n", evalRat(&state, player));
        fprintf(stderr, "@@@@@@@@\nHave %d moves to explore for player %d\n", state.numLegalMoves, player);

        for (int i = 0; i < state.numLegalMoves; i++)
        {
            if (timeup())
                return;

            State newState;
            memcpy(&newState, &state, sizeof(struct State));
            performMove(&newState, numlegalmoves[i]);

            double score = minmax_ab(newState, player, depth, -DBL_MAX, DBL_MAX) + evalRat(&newState, player);
            fprintf(stderr, "Score for move %d is %f\n", numlegalmoves[i], score);
            if (score >= bestMoveScore)
            {
                bestMoveScore = score;
                bestmoveatdepth = numlegalmoves[i];
                bestmovesarr[bestmovescount++] = numlegalmoves[i];
            }
        }
        fprintf(stderr, "Best move for this depth is %d\n", bestmoveatdepth);
        if (bestmoveindex != NULL)
        {
            *bestmoveindex = bestmoveatdepth;
        }
        else
            fprintf(stderr, "The bestmoveindex ptr is null. This should not happen.\n");

        fprintf(stderr, "------\nSelecting move %d as the best move for player %d with score %f at depth %d\n-----\n", *bestmoveindex, player, bestMoveScore, depth);
    }
}

// Just responsible for actually triggering the IDS thread
// And returning results when done.
void FindBestMove(int player, char board[8][8], char *bestmove)
{
    // There are some weights I read off of a file (params.txt), and try to optimize them there.
    // int piecediff;
    // int exposeddiff;
    // int clusterscore;
    // int kingsdiff;
    // int centercount;
    // int stuckcount;
    // int backrowmultiplier;
    reset_timer(&timer);

    FILE *fp = fopen("./params.txt", "r+");
    if (fp == NULL)
    {
        fprintf(stderr, "Couldn't load params file.");
        exit(-1);
    }
    else
    {
        // Read the file.
        fscanf(fp, "%lf %lf %lf %lf %lf %lf %lf", &piecediff, &exposeddiff, &clusterscore, &kingsdiff, &centercount, &stuckcount, &backrowmultiplier);
        fprintf(stderr, "Initial params:\n-----\n piece_diff: %f \n exposed_diff: %f \n cluster_score: %f \n kings_diff: %f \n center_count: %f\n stuck_count: %f \n backrow_mul: %f \n ----- \n", piecediff, exposeddiff, clusterscore, kingsdiff, centercount, stuckcount, backrowmultiplier);
    }
    fclose(fp);

    struct State state;
    setupBoardState(&state, player, board);

    fprintf(stderr, "Printing board state test:\n");
    printBoard(&state);

    // Create args.
    arguments.bestMove = &ids_retval;
    arguments.player = player;
    arguments.state = state;
    arguments.start = start;

    id_search();
    fprintf(stderr, "ids search returned %d as best move.\n-----\n", ids_retval);
    safeCopy(bestmove, state.movelist[ids_retval], MaxMoveLength, MoveLength(state.movelist[ids_retval]));
}