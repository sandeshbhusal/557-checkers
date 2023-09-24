import java.util.*;
import java.io.*;

public class PlayerHelper {
    public static final int MaxMoveLength=12;
    public static final int Clear = 0x1f;
    public static final int Empty = 0x00;
    public static final int Piece = 0x20;
    public static final int King = 0x60;
    public static final int Red = 0x00;
    public static final int White = 0x80;

    static float SecPerMove;
    static char[][] board = new char[8][8];
    static char[] bestmove = new char[MaxMoveLength];
    static int me, cutoff, endgame;
    static long NumNodes;
    static int MaxDepth;

    /*** For the jump list ***/
    static int jumpptr = 0;
    static int jumplist[][] = new int[48][MaxMoveLength];

    /*** For the move list ***/
    static int numLegalMoves = 0;
    static int movelist[][] = new int[48][MaxMoveLength];

    static Random random = new Random();
    static int moveCount = 0;
    static double homerowValue = 1.0;

    public static int number(char x) {
        return ((x) & 0x1f);
    }

    public static boolean empty(char x) {
        return ((((x) >> 5) & 0x03) == 0 ? 1 : 0) != 0;
    }

    public static boolean piece(char x) {
        return ((((x) >> 5) & 0x03) == 1 ? 1 : 0) != 0;
    }

    public static boolean KING(char x) {
        return king(x);
    }

    public static boolean king(char x) {
        return ((((x) >> 5) & 0x03) == 3 ? 1 : 0) != 0;
    }

    public static int color(char x) {
        return ((((x) >> 7) & 1) + 1);
    }

    public static void memcpy(char[][] dest, char[][] src) {
        for (int x = 0; x < 8; x++)
            for (int y = 0; y < 8; y++)
                dest[x][y] = src[x][y];
    }

    public static void memcpy(char[] dest, char[] src, int num) {
        for(int x=0;x<dest.length;x++) dest[x]=0;
        for (int x = 0; x < num; x++)
            dest[x] = src[x];
    }

    public static void memset(char[] arr, int val, int num) {
        for (int x = 0; x < num; x++)
            arr[x] = (char) val;
    }

    /* Copy a square state */
    static char CopyState(char dest, char src) {
        char state;

        dest &= Clear;
        state = (char) (src & 0xE0);
        dest |= state;
        return dest;
    }

    /* Reset board to initial configuration */
    static void ResetBoard() {
        int x, y;
        char pos;

        pos = 0;
        for (y = 0; y < 8; y++)
            for (x = 0; x < 8; x++) {
                if (x % 2 != y % 2) {
                    board[y][x] = pos;
                    if (y < 3 || y > 4)
                        board[y][x] |= Piece;
                    else
                        board[y][x] |= Empty;
                    if (y < 3)
                        board[y][x] |= Red;
                    if (y > 4)
                        board[y][x] |= White;
                    pos++;
                } else
                    board[y][x] = 0;
            }
        endgame = 0;
    }

    /* Add a move to the legal move list */
    static void AddMove(char move[]) {
        int i;

        for (i = 0; i < MaxMoveLength; i++)
            movelist[numLegalMoves][i] = move[i];
        numLegalMoves++;
    }

    /* Finds legal non-jump moves for the King at position x,y */
    static void FindKingMoves(char board[][], int x, int y) {
        int i, j, x1, y1;
        char move[] = new char[MaxMoveLength];

        memset(move, 0, MaxMoveLength);

        /* Check the four adjacent squares */
        for (j = -1; j < 2; j += 2)
            for (i = -1; i < 2; i += 2) {
                y1 = y + j;
                x1 = x + i;
                /* Make sure we're not off the edge of the board */
                if (y1 < 0 || y1 > 7 || x1 < 0 || x1 > 7)
                    continue;
                if (empty(board[y1][x1])) { /* The square is empty, so we can move there */
                    move[0] = (char) (number(board[y][x]) + 1);
                    move[1] = (char) (number(board[y1][x1]) + 1);
                    AddMove(move);
                }
            }
    }

    /* Finds legal non-jump moves for the Piece at position x,y */
    static void FindMoves(int player, char board[][], int x, int y) {
        int i, j, x1, y1;
        char move[] = new char[MaxMoveLength];

        memset(move, 0, MaxMoveLength);

        /* Check the two adjacent squares in the forward direction */
        if (player == 1)
            j = 1;
        else
            j = -1;
        for (i = -1; i < 2; i += 2) {
            y1 = y + j;
            x1 = x + i;
            /* Make sure we're not off the edge of the board */
            if (y1 < 0 || y1 > 7 || x1 < 0 || x1 > 7)
                continue;
            if (empty(board[y1][x1])) { /* The square is empty, so we can move there */
                move[0] = (char) (number(board[y][x]) + 1);
                move[1] = (char) (number(board[y1][x1]) + 1);
                AddMove(move);
            }
        }
    }

    /* Adds a jump sequence the the legal jump list */
    static void AddJump(char move[]) {
        int i;

        for (i = 0; i < MaxMoveLength; i++)
            jumplist[jumpptr][i] = move[i];
        jumpptr++;
    }

    /* Finds legal jump sequences for the King at position x,y */
    static int FindKingJump(int player, char board[][], char move[], int len, int x, int y) {
        int i, j, x1, y1, x2, y2, FoundJump = 0;
        char one, two;
        char mymove[] = new char[MaxMoveLength];
        char myboard[][] = new char[8][8];

        memcpy(mymove, move, MaxMoveLength);

        /* Check the four adjacent squares */
        for (j = -1; j < 2; j += 2)
            for (i = -1; i < 2; i += 2) {
                y1 = y + j;
                x1 = x + i;
                y2 = y + 2 * j;
                x2 = x + 2 * i;
                /* Make sure we're not off the edge of the board */
                if (y2 < 0 || y2 > 7 || x2 < 0 || x2 > 7)
                    continue;
                one = board[y1][x1];
                two = board[y2][x2];
                /*
                 * If there's an enemy piece adjacent, and an empty square after hum, we can
                 * jump
                 */
                if (!empty(one) && color(one) != player && empty(two)) {
                    /* Update the state of the board, and recurse */
                    memcpy(myboard, board);
                    myboard[y][x] &= Clear;
                    myboard[y1][x1] &= Clear;
                    mymove[len] = (char) (number(board[y2][x2]) + 1);
                    FoundJump = FindKingJump(player, myboard, mymove, len + 1, x + 2 * i, y + 2 * j);
                    if (FoundJump == 0) {
                        FoundJump = 1;
                        AddJump(mymove);
                    }
                }
            }
        return FoundJump;
    }

    /* Finds legal jump sequences for the Piece at position x,y */
    static int FindJump(int player, char board[][], char move[], int len, int x, int y) {
        int i, j, x1, y1, x2, y2, FoundJump = 0;
        char one, two;
        char mymove[] = new char[MaxMoveLength];
        char myboard[][] = new char[8][8];

        memcpy(mymove, move, MaxMoveLength);

        /* Check the two adjacent squares in the forward direction */
        if (player == 1)
            j = 1;
        else
            j = -1;
        for (i = -1; i < 2; i += 2) {
            y1 = y + j;
            x1 = x + i;
            y2 = y + 2 * j;
            x2 = x + 2 * i;
            /* Make sure we're not off the edge of the board */
            if (y2 < 0 || y2 > 7 || x2 < 0 || x2 > 7)
                continue;
            one = board[y1][x1];
            two = board[y2][x2];
            /*
             * If there's an enemy piece adjacent, and an empty square after him, we can
             * jump
             */
            if (!empty(one) && color(one) != player && empty(two)) {
                /* Update the state of the board, and recurse */
                memcpy(myboard, board);
                myboard[y][x] &= Clear;
                myboard[y1][x1] &= Clear;
                mymove[len] = (char) (number(board[y2][x2]) + 1);
                FoundJump = FindJump(player, myboard, mymove, len + 1, x + 2 * i, y + 2 * j);
                if (FoundJump == 0) {
                    FoundJump = 1;
                    AddJump(mymove);
                }
            }
        }
        return FoundJump;
    }

    /* Determines all of the legal moves possible for a given state */
    static int FindLegalMoves(State state) {
        int x, y;
        char move[] = new char[MaxMoveLength], board[][] = new char[8][8];

        memset(move, 0, MaxMoveLength);
        jumpptr = numLegalMoves = 0;
        memcpy(board, state.board);

        /* Loop through the board array, determining legal moves/jumps for each piece */
        for (y = 0; y < 8; y++)
            for (x = 0; x < 8; x++) {
                if (x % 2 != y % 2 && color(board[y][x]) == state.player && !empty(board[y][x])) {
                    if (KING(board[y][x])) { /* King */
                        move[0] = (char) (number(board[y][x]) + 1);
                        FindKingJump(state.player, board, move, 1, x, y);
                        if (jumpptr == 0)
                            FindKingMoves(board, x, y);
                    } else if (piece(board[y][x])) { /* Piece */
                        move[0] = (char) (number(board[y][x]) + 1);
                        FindJump(state.player, board, move, 1, x, y);
                        if (jumpptr == 0)
                            FindMoves(state.player, board, x, y);
                    }
                }
            }
        if (jumpptr != 0) {
            for (x = 0; x < jumpptr; x++)
                for (y = 0; y < MaxMoveLength; y++)
                    state.movelist[x][y] = (char) (jumplist[x][y]);
            state.numLegalMoves = jumpptr;
        } else {
            for (x = 0; x < numLegalMoves; x++)
                for (y = 0; y < MaxMoveLength; y++)
                    state.movelist[x][y] = (char) (movelist[x][y]);
            state.numLegalMoves = numLegalMoves;
        }
        return (jumpptr + numLegalMoves);
    }

    /* Converts a square label to it's x,y position */
    static void NumberToXY(char num, int[] xy) {
        int i = 0, newy, newx;

        for (newy = 0; newy < 8; newy++)
            for (newx = 0; newx < 8; newx++) {
                if (newx % 2 != newy % 2) {
                    i++;
                    if (i == (int) num) {
                        xy[0] = newx;
                        xy[1] = newy;
                        return;
                    }
                }
            }
        xy[0] = 0;
        xy[1] = 0;
    }

    /* Returns the length of a move */
    static int MoveLength(char move[]) {
        int i;

        i = 0;
        while (i < MaxMoveLength && move[i] != 0)
            i++;
        return i;
    }

    /* Converts the text version of a move to its integer array version */
    static int TextToMove(String mtext, char[] move) {
        int len = 0, last;
        char val;
        String num;

        System.err.println("Move text = " + mtext);

        for (int i = 0; i < mtext.length() && mtext.charAt(i) != '\0';) {
            last = i;
            while (i < mtext.length() && mtext.charAt(i) != '\0' && mtext.charAt(i) != '-')
                i++;

            num = mtext.substring(last, i);
            System.err.println("num = " + num);
            val = (char) Integer.parseInt(num);

            if (val <= 0 || val > 32)
                return 0;
            move[len] = val;
            len++;
            if (i < mtext.length() && mtext.charAt(i) != '\0')
                i++;
        }
        if (len < 2 || len > MaxMoveLength)
            return 0;
        else
            return len;
    }

    /* Converts the integer array version of a move to its text version */
    static String MoveToText(char move[]) {
        int i;
        char temp[] = new char[8];

        String mtext = "";
        if (move[0] != 0) {
            mtext += ((int) (move[0]));
            for (i = 1; i < MaxMoveLength; i++) {
                if (move[i] != 0) {
                    mtext += "-";
                    mtext += ((int) (move[i]));
                }
            }
        }
        return mtext;
    }

    /* Performs a move on the board, updating the state of the board */
    static void PerformMove(char board[][], char move[], int mlen) {
        int i, j, x, y, x1, y1, x2, y2;

        int xy[] = new int[2];

        NumberToXY(move[0], xy);
        x = xy[0];
        y = xy[1];
        NumberToXY(move[mlen - 1], xy);
        x1 = xy[0];
        y1 = xy[1];
        board[y1][x1] = CopyState(board[y1][x1], board[y][x]);
        if (y1 == 0 || y1 == 7)
            board[y1][x1] |= King;
        board[y][x] &= Clear;
        NumberToXY(move[1], xy);
        x2 = xy[0];
        y2 = xy[1];
        if (Math.abs(x2 - x) == 2) {
            for (i = 0, j = 1; j < mlen; i++, j++) {
                if (move[i] > move[j]) {
                    y1 = -1;
                    if ((move[i] - move[j]) == 9)
                        x1 = -1;
                    else
                        x1 = 1;
                } else {
                    y1 = 1;
                    if ((move[j] - move[i]) == 7)
                        x1 = -1;
                    else
                        x1 = 1;
                }
                NumberToXY(move[i], xy);
                x = xy[0];
                y = xy[1];
                board[y + y1][x + x1] &= Clear;
            }
        }
    }

    public static void main(String argv[]) throws Exception {
        // System.err.println("AAAAA");
        if (argv.length >= 2)
            System.err.println("Argument:" + argv[1]);
        //PlayerHelper stupid = new PlayerHelper();
        play(argv);
    }

    static String myRead(BufferedReader br, int y) {
        String rval = "";
        char line[] = new char[1000];
        int x, len = 0;
        // System.err.println("Java waiting for input");
        while(len==0)
        {
            try {
                len = br.read(line, 0, y);
            } catch (Exception e) {
                System.err.println("Java wio exception");
            }
            for (x = 0; x < len; x++) rval += line[x];
            rval=rval.trim();
            len=rval.length();
        }
        System.err.println("Java read " + len + " chars: " + rval);
        return rval;
    }

    static String myRead(BufferedReader br) {
        String rval = "";
        char line[] = new char[1000];
        int x, len = 0;
        // System.err.println("Java waiting for input");
        while(len==0)
        {
            try {
                // while(!br.ready()) ;
                len = br.read(line, 0, 1000);
            } catch (Exception e) {
                System.err.println("Java wio exception");
            }
            for (x = 0; x < len; x++) rval += line[x];
            rval=rval.trim();
            len=rval.length();
        }
        System.err.println("Java wRead " + rval);
        return rval;
    }

    // count red only, for debugging. 
    public static int countRedPieces(char[][] curBoard) {
        int pieces = 0;
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                if (x % 2 != y % 2) {
                    if (color(curBoard[y][x]) == 1) { //
                    
                        if (KING(curBoard[y][x]) | piece(curBoard[y][x])) {
                            pieces += 1;
                        }
                    }
                }
            }
        }
        return pieces;
    }

    public static int countWhitePieces(char[][] curBoard) {
        int pieces = 0;
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                if (x % 2 != y % 2) {
                    if (color(curBoard[y][x]) == 2) { //
                    
                        if (KING(curBoard[y][x]) | piece(curBoard[y][x])) {
                            pieces += 1;
                        }
                    }
                }
            }
        }
        return pieces;
    }

    public static void play(String argv[]) throws Exception {
        char move[] = new char[MaxMoveLength];
        int mlen, player1;
        String buf;

        BufferedReader br = new BufferedReader(new InputStreamReader(System.in));

        /* Convert command line parameters */
        System.err.println("time limit = " + argv[0]);
        SecPerMove = (float) (Double.parseDouble(argv[0]));


        /* Determine if I am player 1 (red) or player 2 (white) */
        // buf = br.readLine();
        buf = myRead(br, 7);
        if (buf.startsWith("Player1")) {
            System.err.println("Java is player 1. ");
            player1 = 1;
        } else {
            System.err.println("Java is player 2");
            player1 = 0;
        }
        if (player1 == 1)
            me = 1;
        else
            me = 2;

        /* Set up the board */
        ResetBoard();

        // explicitly handling first move of each case.
        if (player1 == 1) {
            /* Find my move, update board, and write move to pipe */

            Player.FindBestMove(1,board,bestmove);
  
            if (bestmove[0] != 0) { /* There is a legal move */
                mlen = MoveLength(bestmove);
                PerformMove(board, bestmove, mlen);

                buf = MoveToText(bestmove);
            } else {
                System.exit(1);
            } /* No legal moves available, so I have lost */

            /* Write the move to the pipe */
            System.err.println("Java making first move: " + buf);
            System.out.println(buf);
        }

        else { // we are player 2. so wait for computer move, then move.
            buf = myRead(br);

            memset(move, 0, MaxMoveLength);

            /* Update the board to reflect opponents move */
            mlen = TextToMove(buf, move);
            PerformMove(board, move, mlen);
            Player.FindBestMove(2,board,bestmove);
            if (bestmove[0] != 0) { /* There is a legal move */
                mlen = MoveLength(bestmove);
                PerformMove(board, bestmove, mlen);
                buf = MoveToText(bestmove);
                // System.err.println("Tanner is here");

            } else
                System.exit(1); /* No legal moves available, so I have lost */

            /* Write the move to the pipe */

            System.err.println("Java moving second: " + buf);
            System.out.println(buf);
        }
        moveCount = 1;
        for (;;) {
            /* Read the other player's move from the pipe */
            // buf=br.readLine();

            
            
            buf = myRead(br);

            memset(move, 0, MaxMoveLength);

            /* Update the board to reflect opponents move */
            mlen = TextToMove(buf, move);
            PerformMove(board, move, mlen);

            /* Find my move, update board, and write move to pipe */
            if (player1 != 0)
                Player.FindBestMove(1,board,bestmove);
            else
                Player.FindBestMove(2,board,bestmove);
            if (bestmove[0] != 0) { /* There is a legal move */
                mlen = MoveLength(bestmove);
                PerformMove(board, bestmove, mlen);
                buf = MoveToText(bestmove);

                

                moveCount+=1;
                // System.err.println(" ------ Move count: " + moveCount + " -----------");
                // System.err.println("is ? ? endgame: " + endgame);
                if(moveCount > 15 ){
                    endgame=1;
                                        
                    // incrementally abandon homerow heuristic into lategame. 
                    // Motivate player to advance pieces off homerow in end game
                    if(homerowValue>=0.1){
                        homerowValue-=0.1; 
                        
                    }else{
                        homerowValue=0;
                    }
                    // System.err.println(" homerow value is : " + homerowValue);
                    
                }

            } else
                System.exit(1); /* No legal moves available, so I have lost */

            /* Write the move to the pipe */
            System.err.println("Java move: " + buf);
            System.out.println(buf);
        }
    }
}
