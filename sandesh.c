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
    double p1piece = 0;
    double p2piece = 0;
    int p1kings = 0;
    int p2kings = 0;
    int p1center = 0;
    int p2center = 0;
    int p1stuckkings = 0;
    int p2stuckkings = 0;
    int p1exposed = 0;
    int p2exposed = 0;

    int p1backrow = 0;
    int p2backrow = 0;

    int p1positionsx[12];
    int p2positionsx[12];
    int p1positionsy[12];
    int p2positionsy[12];

    int p1posindex = 0, p2posindex = 0;

    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            if (x % 2 != y % 2 && !empty(state->board[y][x]))
            {
                if (color(state->board[y][x]) == 1)
                {
                    p1positionsx[p1posindex++] = x;
                    p1positionsy[p1posindex++] = y;
                    if (y == 0)
                        p1backrow += 5;
                    if (y == 1)
                        p1backrow += 2;

                    if (king(state->board[y][x]))
                    {
                        p1kings++;
                        if (y == 3 || y == 5)
                            p1center += 3;

                        if (y > 5 && (x < 2 || x > 5))
                            p1stuckkings += 1;
                    }
                    else
                    {
                        p1piece++;
                        if (y == 3 || y == 5)
                            p1center += 2;
                    }
                    if (isExposed(state->board, x, y, 1))
                        p1exposed += 1;
                }
                else
                {
                    p2positionsx[p2posindex++] = x;
                    p2positionsy[p2posindex++] = y;
                    if (y == 7)
                        p2backrow += 5;
                    if (y == 6)
                        p2backrow += 2;

                    if (king(state->board[y][x]))
                    {
                        p2kings++;
                        if (y == 3 || y == 5)
                            p2center += 3;
                        if (y < 2 && (x < 2 || x > 5))
                            p2stuckkings += 1;
                    }
                    else
                    {
                        p2piece++;
                        if (y == 3 || y == 5)
                            p2center += 2;
                    }
                    if (isExposed(state->board, x, y, 2))
                        p2exposed += 1;
                }
            }
        }
    }

    // Run a loop to calculate distance between friend pieces,
    // so that they can "come closer".
    int p1diff = 0;
    int p2diff = 0;
    for (int i = 0; i < p1posindex; i++)
        for (int j = 0; j < p1posindex; j++)
            p1diff += (p1positionsx[j] - p1positionsx[i]) * (p1positionsx[j] - p1positionsx[i]) + (p1positionsy[j] - p1positionsy[i]) * (p1positionsy[j] - p1positionsy[i]);

    for (int i = 0; i < p2posindex; i++)
        for (int j = 0; j < p2posindex; j++)
            p2diff += (p2positionsx[j] - p2positionsx[i]) * (p2positionsx[j] - p2positionsx[i]) + (p2positionsy[j] - p2positionsy[i]) * (p2positionsy[j] - p2positionsy[i]);

    // Add 1 to each piece count, so that we don't have inf. division.
    p1piece += 1;
    p2piece += 1;

    double score = 0.0;

    if (maxplayer == 1 && p2piece < 6)
        randomseltoggle = 1;
    if (maxplayer == 2 && p1piece < 6)
        randomseltoggle = 1;

    if (randomseltoggle && maxplayer == 1)
        score += p1diff * clusterscore;

    if (randomseltoggle && maxplayer == 2)
        score += p2diff * clusterscore;

    score += ((double)p1piece - (double)p2piece) * piecediff + ((double)p1kings - (double)p2kings) * kingsdiff - ((double)p1exposed - (double)p2exposed) * exposeddiff + ((double)p1backrow - (double)p2backrow) * backrowmultiplier;

    if (maxplayer == 1)
        return score + (double)p1center * centercount - p2kings * 100 + evalSupportPieces(state, 1);
    else
        return -score + (double)p2center * centercount - p1kings * 100 + evalSupportPieces(state, 2);
}

double minmax_ab(struct State state, int maxplayer, int depth, double alpha, double beta)
{
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
void id_search(IDSArgs_t args);

void id_search(IDSArgs_t arguments)
{
    State state = arguments.state;
    int player = arguments.player;
    volatile int *bestmoveindex = arguments.bestMove;

    int depth = 5;
    double bestMoveScore;

    while (depth++ < 7)
    {
        int bestmovesarr[100];
        int bestmovescount = 0;
        bestMoveScore = -DBL_MAX;
        int bestmoveatdepth = 0;

        fprintf(stderr, "Reached depth=%d;\n", depth);
        fprintf(stderr, "Game eval on default: %f\n", evalRat(&state, player));
        fprintf(stderr, "@@@@@@@@\nHave %d moves to explore for player %d\n", state.numLegalMoves, player);

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

        for (int i = 0; i < state.numLegalMoves; i++)
        {
            State newState;
            memcpy(&newState, &state, sizeof(struct State));
            performMove(&newState, numlegalmoves[i]);

            double score = minmax_ab(newState, player, depth, -DBL_MAX, DBL_MAX);
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

    id_search(arguments);
    fprintf(stderr, "ids search returned %d as best move.\n-----\n", ids_retval);
    safeCopy(bestmove, state.movelist[ids_retval], MaxMoveLength, MoveLength(state.movelist[ids_retval]));
}