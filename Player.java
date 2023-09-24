import java.util.*;
import java.io.*;

public class Player {

    static Random random=new Random();

    static void setupBoardState(State state, int player, char[][] board)
    {
        /* Set up the current state */
        state.player = player;
        PlayerHelper.memcpy(state.board,board);

        /* Find the legal moves for the current state */
        PlayerHelper.FindLegalMoves(state);
    }

    
    static void PerformMove(State state, int moveIndex)
    {
        PlayerHelper.PerformMove(state.board, state.movelist[moveIndex], PlayerHelper.MoveLength(state.movelist[moveIndex]));
        state.player = state.player%2+1;
        PlayerHelper.FindLegalMoves(state);
    }


        /* Employ your favorite search to find the best move. This code is an example */
    /* of an alpha/beta search, except I have not provided the MinVal,MaxVal,EVAL */
    /*
     * functions. This example code shows you how to call the FindLegalMoves
     * function
     */
    /* and the PerformMove function */
    public static void FindBestMove(int player, char[][] board, char[] bestmove) {
        int myBestMoveIndex;
        
        State state = new State(); // , nextstate;
        setupBoardState(state, player, board);

        myBestMoveIndex = random.nextInt(state.numLegalMoves);

        for (int x = 0; x < state.numLegalMoves; x++) {
            State nextState = new State(state);
            PerformMove(nextState, x);
            printBoard(nextState);
            System.err.println("Eval of board: " + evalBoard(nextState));
        }
        PlayerHelper.memcpy(bestmove, state.movelist[myBestMoveIndex], PlayerHelper.MoveLength(state.movelist[myBestMoveIndex]));
    }

    static void printBoard(State state)
    {
        int y,x;

        for(y=0; y<8; y++) 
        {
            for(x=0; x<8; x++)
            {
                if(x%2 != y%2)
                {
                     if(PlayerHelper.empty(state.board[y][x]))
                     {
                         System.err.print(" ");
                     }
                     else if(PlayerHelper.king(state.board[y][x]))
                     {
                         if(PlayerHelper.color(state.board[y][x])==2) System.err.print("B");
                         else System.err.print("A");
                     }
                     else if(PlayerHelper.piece(state.board[y][x]))
                     {
                         if(PlayerHelper.color(state.board[y][x])==2) System.err.print("b");
                         else System.err.print("a");
                     }
                }
                else
                {
                    System.err.print("@");
                }
            }
            System.err.print("\n");
        }
    }

    /* An example of how to walk through a board and determine what pieces are on it*/
    static double evalBoard(State state)
    {
        int y,x;
        double score;
        score=0.0;

        for(y=0; y<8; y++) for(x=0; x<8; x++)
        {
            if(x%2 != y%2)
            {
                 if(PlayerHelper.empty(state.board[y][x]))
                 {
                 }
                 else if(PlayerHelper.king(state.board[y][x]))
                 {
                     if(PlayerHelper.color(state.board[y][x])==2) score += 2.0;
                     else score -= 2.0;
                 }
                 else if(PlayerHelper.piece(state.board[y][x]))
                 {
                     if(PlayerHelper.color(state.board[y][x])==2) score += 1.0;
                     else score -= 1.0;
                 }
            }
        }

        if(state.player==1) score = -score;

        return score;

    }

}
