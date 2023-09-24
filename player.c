#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>
#include <time.h>
#include "myprog.h"
#include "defs.h"


/* copy numbytes from src to destination 
   before the copy happens the destSize bytes of the destination array is set to 0s
*/
void safeCopy(char *dest, char *src, int destSize, int numbytes)
{
    memset(dest,0,destSize);
    memcpy(dest,src,numbytes);
}


void printBoard(struct State *state)
{
    int y,x;

    for(y=0; y<8; y++) 
    {
        for(x=0; x<8; x++)
        {
            if(x%2 != y%2)
            {
                 if(empty(state->board[y][x]))
                 {
                     fprintf(stderr," ");
                 }
                 else if(king(state->board[y][x]))
                 {
                     if(color(state->board[y][x])==2) fprintf(stderr,"B");
                     else fprintf(stderr,"A");
                 }
                 else if(piece(state->board[y][x]))
                 {
                     if(color(state->board[y][x])==2) fprintf(stderr,"b");
                     else fprintf(stderr,"a");
                 }
            }
            else
            {
                fprintf(stderr," ");
            }
        }
        fprintf(stderr,"\n");
    }
}



/* An example of how to walk through a board and determine what pieces are on it*/
double evalBoard(struct State *state)
{
    int y,x;
    double score;
    score=0.0;

    for(y=0; y<8; y++) for(x=0; x<8; x++)
    {
        if(x%2 != y%2)
        {
             if(empty(state->board[y][x]))
             {
             }
             else if(king(state->board[y][x]))
             {
                 if(color(state->board[y][x])==2) score += 2.0;
                 else score -= 2.0;
             }
             else if(piece(state->board[y][x]))
             {
                 if(color(state->board[y][x])==2) score += 1.0;
                 else score -= 1.0;
             }
        }
    }

    if(state->player==1) score = -score;

    return score;

}




/* Employ your favorite search to find the best move here.  */
/* This example code shows you how to call the FindLegalMoves function */
/* and the PerformMove function */
void FindBestMove(int player, char board[8][8], char* bestmove)
{
    int bestMoveIndex; 
    struct State state; 

    setupBoardState(&state, player, board);


    //Here's an example loop walking through all the moves in the board state
    for(int i=0;i<state.numLegalMoves;i++)
    {
        State nextState;
        memcpy(&nextState,&state,sizeof(State));
        performMove(&nextState,i);
        printBoard(&nextState);
        fprintf(stderr,"Eval of board: %f\n",evalBoard(&nextState));
    }

    // For now, until you write your search routine, we will just set the best move
    // to be a random (legal) one, so that it plays a legal game of checkers.
    // You *will* want to replace this with a more intelligent move seleciton
    bestMoveIndex = rand()%state.numLegalMoves;


    safeCopy(bestmove, state.movelist[bestMoveIndex],MaxMoveLength, MoveLength(state.movelist[bestMoveIndex]));
}



