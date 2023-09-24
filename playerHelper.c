#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>
#include <time.h>
#include "myprog.h"
#include "defs.h"
 
float SecPerMove;
char board[8][8];
char bestmove[MaxMoveLength];
int me,cutoff,endgame;
long NumNodes;
int MaxDepth;

/*** For timing ***/
clock_t start;
struct tms bff;

/*** For the jump list ***/
int jumpptr = 0;
int jumplist[48][MaxMoveLength];

/*** For the move list ***/
int numLegalMoves = 0;
int movelist[48][MaxMoveLength];

/* Print the amount of time passed since my turn began */
void PrintTime(void)
{
    clock_t current;
    float total;

    current = times(&bff);
    total = (float) ((float)current-(float)start)/CLK_TCK;
    fprintf(stderr, "Time = %f\n", total);
}

/* Copy a square state */
void CopyState(char *dest, char src)
{
    char state;
    
    *dest &= Clear;
    state = src & 0xE0;
    *dest |= state;
}

/* Reset board to initial configuration */
void ResetBoard(void)
{
        int x,y;
    char pos;

        pos = 0;
        for(y=0; y<8; y++)
        for(x=0; x<8; x++)
        {
                if(x%2 != y%2) {
                        board[y][x] = pos;
                        if(y<3 || y>4) board[y][x] |= Piece; else board[y][x] |= Empty;
                        if(y<3) board[y][x] |= Red; 
                        if(y>4) board[y][x] |= White;
                        pos++;
                } else board[y][x] = 0;
        }
    endgame = 0;
}

/* Add a move to the legal move list */
void AddMove(char move[MaxMoveLength])
{
    int i;

    for(i=0; i<MaxMoveLength; i++) movelist[numLegalMoves][i] = move[i];
    numLegalMoves++;
}

/* Finds legal non-jump moves for the King at position x,y */
void FindKingMoves(char board[8][8], int x, int y) 
{
    int i,j,x1,y1;
    char move[MaxMoveLength];

    memset(move,0,MaxMoveLength*sizeof(char));

    /* Check the four adjacent squares */
    for(j=-1; j<2; j+=2)
    for(i=-1; i<2; i+=2)
    {
        y1 = y+j; x1 = x+i;
        /* Make sure we're not off the edge of the board */
        if(y1<0 || y1>7 || x1<0 || x1>7) continue; 
        if(empty(board[y1][x1])) {  /* The square is empty, so we can move there */
            move[0] = number(board[y][x])+1;
            move[1] = number(board[y1][x1])+1;    
            AddMove(move);
        }
    }
}

/* Finds legal non-jump moves for the Piece at position x,y */
void FindMoves(int player, char board[8][8], int x, int y) 
{
    int i,j,x1,y1;
    char move[MaxMoveLength];

    memset(move,0,MaxMoveLength*sizeof(char));

    /* Check the two adjacent squares in the forward direction */
    if(player == 1) j = 1; else j = -1;
    for(i=-1; i<2; i+=2)
    {
        y1 = y+j; x1 = x+i;
        /* Make sure we're not off the edge of the board */
        if(y1<0 || y1>7 || x1<0 || x1>7) continue; 
        if(empty(board[y1][x1])) {  /* The square is empty, so we can move there */
            move[0] = number(board[y][x])+1;
            move[1] = number(board[y1][x1])+1;    
            AddMove(move);
        }
    }
}

/* Adds a jump sequence the the legal jump list */
void AddJump(char move[MaxMoveLength])
{
    int i;
    
    for(i=0; i<MaxMoveLength; i++) jumplist[jumpptr][i] = move[i];
    jumpptr++;
}

/* Finds legal jump sequences for the King at position x,y */
int FindKingJump(int player, char board[8][8], char move[MaxMoveLength], int len, int x, int y) 
{
    int i,j,x1,y1,x2,y2,FoundJump = 0;
    char one,two,mymove[MaxMoveLength],myboard[8][8];

    memcpy(mymove,move,MaxMoveLength*sizeof(char));

    /* Check the four adjacent squares */
    for(j=-1; j<2; j+=2)
    for(i=-1; i<2; i+=2)
    {
        y1 = y+j; x1 = x+i;
        y2 = y+2*j; x2 = x+2*i;
        /* Make sure we're not off the edge of the board */
        if(y2<0 || y2>7 || x2<0 || x2>7) continue; 
        one = board[y1][x1];
        two = board[y2][x2];
        /* If there's an enemy piece adjacent, and an empty square after hum, we can jump */
        if(!empty(one) && color(one) != player && empty(two)) {
            /* Update the state of the board, and recurse */
            memcpy(myboard,board,64*sizeof(char));
            myboard[y][x] &= Clear;
            myboard[y1][x1] &= Clear;
            mymove[len] = number(board[y2][x2])+1;
            FoundJump = FindKingJump(player,myboard,mymove,len+1,x+2*i,y+2*j);
            if(!FoundJump) {
                FoundJump = 1;
                AddJump(mymove);
            }
        }
    }
    return FoundJump;
}

/* Finds legal jump sequences for the Piece at position x,y */
int FindJump(int player, char board[8][8], char move[MaxMoveLength], int len, int x, int y) 
{
    int i,j,x1,y1,x2,y2,FoundJump = 0;
    char one,two,mymove[MaxMoveLength],myboard[8][8];

    memcpy(mymove,move,MaxMoveLength*sizeof(char));

    /* Check the two adjacent squares in the forward direction */
    if(player == 1) j = 1; else j = -1;
    for(i=-1; i<2; i+=2)
    {
        y1 = y+j; x1 = x+i;
        y2 = y+2*j; x2 = x+2*i;
        /* Make sure we're not off the edge of the board */
        if(y2<0 || y2>7 || x2<0 || x2>7) continue; 
        one = board[y1][x1];
        two = board[y2][x2];
        /* If there's an enemy piece adjacent, and an empty square after hum, we can jump */
        if(!empty(one) && color(one) != player && empty(two)) {
            /* Update the state of the board, and recurse */
            memcpy(myboard,board,64*sizeof(char));
            myboard[y][x] &= Clear;
            myboard[y1][x1] &= Clear;
            mymove[len] = number(board[y2][x2])+1;
            FoundJump = FindJump(player,myboard,mymove,len+1,x+2*i,y+2*j);
            if(!FoundJump) {
                FoundJump = 1;
                AddJump(mymove);
            }
        }
    }
    return FoundJump;
}

/* Determines all of the legal moves possible for a given state */
int FindLegalMoves(struct State *state)
{
    int x,y;
    char move[MaxMoveLength], board[8][8];

    memset(move,0,MaxMoveLength*sizeof(char));
    jumpptr = numLegalMoves = 0;
    memcpy(board,state->board,64*sizeof(char));

    /* Loop through the board array, determining legal moves/jumps for each piece */
    for(y=0; y<8; y++)
    for(x=0; x<8; x++)
    {
        if(x%2 != y%2 && color(board[y][x]) == state->player && !empty(board[y][x])) {
            if(king(board[y][x])) { /* King */
                move[0] = number(board[y][x])+1;
                FindKingJump(state->player,board,move,1,x,y);
                if(!jumpptr) FindKingMoves(board,x,y);
            } 
            else if(piece(board[y][x])) { /* Piece */
                move[0] = number(board[y][x])+1;
                FindJump(state->player,board,move,1,x,y);
                if(!jumpptr) FindMoves(state->player,board,x,y);    
            }
        }    
    }
    if(jumpptr) {
        for(x=0; x<jumpptr; x++) 
        for(y=0; y<MaxMoveLength; y++) 
        state->movelist[x][y] = jumplist[x][y];
        state->numLegalMoves = jumpptr;
    } 
    else {
        for(x=0; x<numLegalMoves; x++) 
        for(y=0; y<MaxMoveLength; y++) 
        state->movelist[x][y] = movelist[x][y];
        state->numLegalMoves = numLegalMoves;
    }
    return (jumpptr+numLegalMoves);
}


/* Converts a square label to it's x,y position */
void NumberToXY(char num, int *x, int *y)
{
    int i=0,newy,newx;

    for(newy=0; newy<8; newy++)
    for(newx=0; newx<8; newx++)
    {
        if(newx%2 != newy%2) {
            i++;
            if(i==(int) num) {
                *x = newx;
                *y = newy;
                return;
            }
        }
    }
    *x = 0; 
    *y = 0;
}

/* Returns the length of a move */
int MoveLength(char move[MaxMoveLength])
{
    int i;

    i = 0;
    while(i<MaxMoveLength&& move[i]) i++;
    return i;
}    

/* Converts the text version of a move to its integer array version */
int TextToMove(char *mtext, char move[MaxMoveLength])
{
    int i=0,len=0,last;
    char val,num[64];

    while(mtext[i] != '\0') {
        last = i;
        while(mtext[i] != '\0' && mtext[i] != '-') i++;
        strncpy(num,&mtext[last],i-last);
        num[i-last] = '\0';
        val = (char) atoi(num);
        if(val <= 0 || val > 32) return 0;
        move[len] = val;
        len++;
        if(mtext[i] != '\0') i++;
    }
    if(len<2 || len>MaxMoveLength) return 0; else return len;
}

/* Converts the integer array version of a move to its text version */
void MoveToText(char move[MaxMoveLength], char *mtext)
{
    int i;
    char temp[8];

    mtext[0] = '\0';
    for(i=0; i<MaxMoveLength; i++) {
        if(move[i]) {
            sprintf(temp,"%d",(int)move[i]);
            strcat(mtext,temp);
            strcat(mtext,"-");
        }
    }
    mtext[strlen(mtext)-1] = '\0';
}

/* copy the board into the board state, and determine the legal moves 
   for player.  

   Returns with the boardState containing the current
   board, the player who's turn it is, and the legal moves that are
   available to the player 
*/
void setupBoardState(struct State *state, int player, char board[8][8])
{
    /* Set up the current state */
    state->player = player;
    memcpy(state->board,board,64*sizeof(char));

    /* Find the legal moves for the current state */
    FindLegalMoves(state);
}

/* Performs the chosen move on the state structure.

   updates the board state with the move
   switches the player to the next player
   finds the legal moves for the next player
*/
void performMove(State *state, int moveIndex)
{
    PerformMove(state->board, state->movelist[moveIndex], MoveLength(state->movelist[moveIndex]));
    state->player = state->player%2 + 1;
    FindLegalMoves(state);
}

/* Performs a move on the board, updating the state of the board */
void PerformMove(char board[8][8], char move[MaxMoveLength], int mlen)
{
    int i,j,x,y,x1,y1,x2,y2;

    NumberToXY(move[0],&x,&y);
    NumberToXY(move[mlen-1],&x1,&y1);
    CopyState(&board[y1][x1],board[y][x]);
    if(y1 == 0 || y1 == 7) board[y1][x1] |= King;
    board[y][x] &= Clear;
    NumberToXY(move[1],&x2,&y2);
    if(abs(x2-x) == 2) {
        for(i=0,j=1; j<mlen; i++,j++) {
            if(move[i] > move[j]) {
                y1 = -1; 
                if((move[i]-move[j]) == 9) x1 = -1; else x1 = 1;
            }
            else {
                y1 = 1;
                if((move[j]-move[i]) == 7) x1 = -1; else x1 = 1;
            }
            NumberToXY(move[i],&x,&y);
            board[y+y1][x+x1] &= Clear;
        }
    }
}


int main(int argc, char *argv[])
{
    char buf[1028],move[MaxMoveLength];
    int mlen,player1;

    /* Convert command line parameters */
    SecPerMove = (float) atof(argv[1]); /* Time allotted for each move */
    MaxDepth = (argc == 3) ? atoi(argv[2]) : -1;

    fprintf(stderr, "Maximum search depth = %i\n", MaxDepth);

    /* Determine if I am player 1 (red) or player 2 (white) */
    fgets(buf, sizeof(buf), stdin);
    if(!strcmp(buf,"Player1\n")) player1 = 1; else player1 = 0;
    if(player1) me = 1; else me = 2;

    /* Set up the board */ 
    ResetBoard();
    srand((unsigned int)time(0));

    if (player1) {
        start = times(&bff);
        goto determine_next_move;
    }

    for(;;) {
        /* Read the other player's move from the pipe */
        fgets(buf, sizeof(buf), stdin);
        start = times(&bff);
        memset(move,0,MaxMoveLength*sizeof(char));

        /* Update the board to reflect opponents move */
        mlen = TextToMove(buf,move);
        PerformMove(board,move,mlen);
        
determine_next_move:
        /* Find my move, update board, and write move to pipe */
        if(player1) FindBestMove(1,board,bestmove); else FindBestMove(2,board,bestmove);
        if(bestmove[0] != 0) { /* There is a legal move */
            mlen = MoveLength(bestmove);
            PerformMove(board,bestmove,mlen);
            MoveToText(bestmove,buf);
        }
        else exit(1); /* No legal moves available, so I have lost */

        /* Write the move to the pipe */
        printf("%s\n", buf);
        fflush(stdout);
    }

    return 0;
}


