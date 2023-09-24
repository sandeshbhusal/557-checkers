import java.util.*;
import java.io.*;

public class State {
    int player;
    char board[][] = new char[8][8];
    char movelist[][] = new char[48][12]; /*
                                           * The following comments were added by Tim Andersen Here Scott has set the
                                           * maximum number of possible legal moves to be 48. Be forewarned that this
                                           * number might not be correct, even though I'm pretty sure it is. The second
                                           * array subscript is (supposed to be) the maximum number of squares that a
                                           * piece could visit in a single move. This number is arrived at be
                                           * recognizing that an opponent will have at most 12 pieces on the board, and
                                           * so you can at most jump twelve times. However, the number really ought to
                                           * be 13 since we also have to record the starting position as part of the
                                           * move sequence. I didn't code this, and I highly doubt that the natural
                                           * course of a game would lead to a person jumping all twelve of an opponents
                                           * checkers in a single move, so I'm not going to change this. I'll leave it
                                           * to the adventuresome to try and devise a way to legally generate a board
                                           * position that would allow such an event. Each move is represented by a
                                           * sequence of numbers, the first number being the starting position of the
                                           * piece to be moved, and the rest being the squares that the piece will visit
                                           * (in order) during the course of a single move. The last number in this
                                           * sequence is the final position of the piece being moved.
                                           */
    int numLegalMoves;

    public State()
    {
    }

    public State(State state)
    {
        PlayerHelper.memcpy(this.board,state.board);
        this.player=state.player;

        for(int i=0;i<movelist.length;i++)
        {
            for(int j=0;j<movelist[i].length;j++)
            {
                this.movelist[i][j] = state.movelist[i][j];
            }
        }
    }
}

