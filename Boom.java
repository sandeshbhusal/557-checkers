import java.util.*;

public class Boom {

    static Random random = new Random();

    static void setupBoardState(State state, int player, char[][] board) {
        /* Set up the current state */
        state.player = player;
        BoomSupport.memcpy(state.board, board);

        /* Find the legal moves for the current state */
        BoomSupport.FindLegalMoves(state);
    }

    static void PerformMove(State state, int moveIndex) {
        BoomSupport.PerformMove(state.board, state.movelist[moveIndex],
                PlayerHelper.MoveLength(state.movelist[moveIndex]));
        state.player = state.player % 2 + 1;
        BoomSupport.FindLegalMoves(state);
    }

    static double minmax(State state, int depth, int maxplayer, double alpha, double beta) {
        if (depth-- == 0)
            return evalBoardForPlayer(state, maxplayer);

        if (state.player == maxplayer) {
            for (int i = 0; i < state.numLegalMoves; i++) {
                State nexState = new State(state);
                PerformMove(nexState, i);

                double score_here = minmax(nexState, depth, maxplayer, alpha, beta);
                alpha = Double.max(alpha, score_here);

                if (beta <= alpha)
                    return beta;
            }

            return alpha;
        } else {
            // score = Double.MAX_VALUE;
            for (int i = 0; i < state.numLegalMoves; i++) {
                State nexState = new State(state);
                PerformMove(nexState, i);

                double score_here = minmax(nexState, depth, maxplayer, alpha, beta);
                beta = Double.min(score_here, beta);

                if (beta <= alpha)
                    return alpha;
            }

            return beta;
        }
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
        double bestMoveScore = -Double.MAX_VALUE;

        State state = new State(); // , nextstate;
        setupBoardState(state, player, board);

        myBestMoveIndex = 0;

        for (int i = 0; i < state.numLegalMoves; i++) {
            State nextState = new State(state);
            PerformMove(nextState, i);

            double score = minmax(nextState, 5, player, -Double.MAX_VALUE, Double.MAX_VALUE);
            if (score > bestMoveScore) {
                bestMoveScore = score;
                myBestMoveIndex = i;
            }
        }

        System.err.printf("Selecting move %d with score %f\n", myBestMoveIndex, bestMoveScore);
        BoomSupport.memcpy(bestmove, state.movelist[myBestMoveIndex],
                PlayerHelper.MoveLength(state.movelist[myBestMoveIndex]));
    }

    static void printBoard(State state) {
        int y, x;

        for (y = 0; y < 8; y++) {
            for (x = 0; x < 8; x++) {
                if (x % 2 != y % 2) {
                    if (BoomSupport.empty(state.board[y][x])) {
                        System.err.print(" ");
                    } else if (BoomSupport.king(state.board[y][x])) {
                        if (BoomSupport.color(state.board[y][x]) == 2)
                            System.err.print("B");
                        else
                            System.err.print("A");
                    } else if (BoomSupport.piece(state.board[y][x])) {
                        if (BoomSupport.color(state.board[y][x]) == 2)
                            System.err.print("b");
                        else
                            System.err.print("a");
                    }
                } else {
                    System.err.print("@");
                }
            }
            System.err.print("\n");
        }
    }

    /*
     * An example of how to walk through a board and determine what pieces are on it
     */
    static double evalBoardForPlayer(State state, int player) {
        int y, x;
        double maxscore;
        maxscore = 0;

        for (y = 0; y < 8; y++)
            for (x = 0; x < 8; x++) {
                if (x % 2 != y % 2 && !BoomSupport.empty(state.board[y][x])) {
                    char ch = state.board[y][x];
                    if (!BoomSupport.empty(ch)) {
                        if (BoomSupport.king(ch))
                            if (BoomSupport.color(ch) == player)
                                maxscore += 2;
                            else
                                maxscore -= 2;
                        else
                            if(BoomSupport.color(ch) == player)
                                maxscore += 1;
                            else 
                                maxscore -= 1;
                    }
                }
            }

        // printBoard(state);
        System.err.println(
                "Score is " + maxscore + " and player is " + state.player + " for maxplayer " + player + "\n--------");
        return maxscore;
    }

    /*
     * An example of how to walk through a board and determine what pieces are on it
     */
    static double evalBoard(State state) {
        int y, x;
        double score;
        score = 0.0;

        for (y = 0; y < 8; y++)
            for (x = 0; x < 8; x++) {
                if (x % 2 != y % 2) {
                    if (BoomSupport.empty(state.board[y][x])) {
                    } else if (BoomSupport.king(state.board[y][x])) {
                        if (BoomSupport.color(state.board[y][x]) == 2)
                            score += 2.0;
                        else
                            score -= 2.0;
                    } else if (BoomSupport.piece(state.board[y][x])) {
                        if (BoomSupport.color(state.board[y][x]) == 2)
                            score += 1.0;
                        else
                            score -= 1.0;
                    }
                }
            }

        if (state.player == 1)
            score = -score;

        return score;

    }

}
