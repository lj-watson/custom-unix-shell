#include "shs.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void printBoard(char board[][26], int n);
bool positionInBounds(int n, int row, int col);
bool checkLegalInDirection(char board[][26], int n, int row, int col,
                           char colour, int deltaRow, int deltaCol);
void flipTiles(char board[][26], int n, int row, int col, char colour, int deltaRow, int deltaCol);
bool skipTurn(char board[][26], int size, char colour);
int numTilesFlipped(char board[][26], int size, int row, int col, char colour, int deltaRow, int deltaCol);
int makeMove(char board[][26], int n, char turn, int *row, int *col);
int playGame(char board[][26], int size, char colour);
bool validMove(char board[][26], int size, int row, int col, char colour);

void reversi(void) {

    pid_t ret_pid = fork();

    if(ret_pid > 0) {
        int status;
        pid_t wret_pid;
        // Wait for child process to finish before returning
        do {
            if((wret_pid = waitpid(ret_pid, &status, WUNTRACED)) == -1) die("waitpid() failed");
        } while(!WIFEXITED(status) && !WIFSIGNALED(status));
        return;
    }

    if(ret_pid == 0) {
        int size = 0, isMoveLegal = 0, whiteCount = 0, blackCount = 0, row = 0, col = 0;
        unsigned char playerColour, computerColour = 'B', rowChar, colChar, turn;
        char board[26][26];
        int indexRow, indexCol;
        int *rowAI = &indexRow;
        int *colAI = &indexCol;
        bool endGame = false;

        printf("Enter the board dimension: ");
        scanf("%d", &size);

        for(int i = 0; i < size; i++) {
            for(int j = 0; j < size; j++)
                board[i][j] = 'U';
        }

        board[(size / 2) - 1][(size / 2) - 1] = 'W';
        board[(size / 2 - 1)][(size / 2)] = 'B';
        board[(size / 2)][(size / 2) - 1] = 'B';
        board[(size / 2)][(size / 2)] = 'W';

        printf("Computer plays (B/W) : ");
        scanf(" %c", &computerColour);

        printBoard(board, size);

        if(computerColour == 'W') {
            playerColour = 'B';
            turn = 'P';
        }
        else {
            playerColour = 'W';
            turn = 'C';
        }

        do{

            //PLAYER'S TURN
            if(turn == 'P' && !skipTurn(board, size, playerColour)) {
                char cont[10];
                printf("Continue playing? Y\\n: ");
                scanf("%s",cont);
                if(strcmp(cont, "n") == 0 || strcmp(cont, "N") == 0) return;
                printf("Enter move for colour %c (RowCol): ", playerColour);
                    
                scanf(" %c %c", &rowChar, &colChar);

                row = (int)rowChar - 97;
                col = (int)colChar - 97;

                if(col < 0 || col > 25 || row < 0 || row > 25 || board[row][col] != 'U')
                    isMoveLegal = 0;

                else if(positionInBounds(size, row, col)) {
                    for (int a = -1; a <= 1; a++) {
                        for (int b = -1; b <= 1; b++) {
                            if(checkLegalInDirection(board, size, row, col, playerColour, a, b)) {
                                flipTiles(board, size, row, col, playerColour, a, b);
                                isMoveLegal = 1;
                            }
                        }
                    }
                }

                if(isMoveLegal == 1) {
                    board[row][col] = playerColour;
                    printBoard(board, size);
                    turn = 'C';
                }
                else {
                    printf("Invalid move.\n");
                    continue;
                }
            }

            else if(turn == 'P'){
                printf("%c player has no valid move.\n", playerColour);
                turn = 'C';
            }

            //COMPUTER'S TURN
            if (turn == 'C' && !skipTurn(board, size, computerColour) && !endGame) {
        
                makeMove(board, size, computerColour, rowAI, colAI);
                
                printf("Computer places %c at %c%c.\n", computerColour, (char)(97 + indexRow), (char)(97 + indexCol));

                board[indexRow][indexCol] = computerColour;
                    
                for (int a = -1; a <= 1; a++) {
                    for (int b = -1; b <= 1; b++) {
                        if(checkLegalInDirection(board, size, indexRow, indexCol, computerColour, a, b)) 
                            flipTiles(board, size, indexRow, indexCol, computerColour, a, b);
                    }
                }

                printBoard(board, size);
                turn = 'P';
            }

            else if (!skipTurn(board, size, playerColour) && !endGame){
                printf("%c player has no valid move.\n", computerColour);
                turn = 'P';
            }

            //END GAME IF THERE ARE NO POSSIBLE PLAYS
            if(skipTurn(board, size, playerColour) && skipTurn(board, size, computerColour))    
                endGame = true;

        } while(!endGame);

        for(int i = 0; i < size; i++) {
            for(int j = 0; j < size; j++) {
                if(board[i][j] == 'W')
                    whiteCount++;
                else if(board[i][j] == 'B')
                    blackCount++;
            }
        }

        if(whiteCount > blackCount)
            printf("W player wins.\n");
        else if(blackCount > whiteCount)
            printf("B player wins.\n");
        else    
            printf("Draw!\n");

        return;
        
    }
    // Fork Error
    else die("fork() failed");
}

void printBoard(char board[][26], int n) {

  char length[n];
  printf("  ");
  for(int i = 0; i < n; i++) {
    length[i] = (97 + i);
    printf("%c", length[i]);
  }

  printf("\n");

  for(int i = 0; i < n; i++) {
    printf("%c ", length[i]);
    for(int j = 0; j < n; j++)
      printf("%c", board[i][j]);
    printf("\n");
  }
}

void flipTiles(char board[][26], int n, int row, int col, char colour, int deltaRow, int deltaCol) {

    char opp = 'W';
    if (colour == 'W')
        opp = 'B';

    int i = row + deltaRow, j = col + deltaCol;

    while(positionInBounds(n, row, col) && board[i][j] == opp) {
        board[i][j] = colour;
        if (deltaRow < 0)
            i--;
        else if (deltaRow > 0)
            i++;
        if (deltaCol < 0)
            j--;
        else if (deltaCol > 0)
            j++; 
    } 
}

bool skipTurn(char board[][26], int size, char colour) {

    for (int i = 0; i < size; i++) {
        for(int j = 0; j < size; j++) {
            if(board[i][j] == 'U') {
                for (int a = -1; a <= 1; a++) {
                    for (int b = -1; b <= 1; b++) {
                        if(checkLegalInDirection(board, size, i, j, colour, a, b)) 
                            return false;
                    }
                }
            }
        }
    }
    return true;
}

bool positionInBounds(int n, int row, int col) {

  if( row < n && col < n && row >= 0 && col >= 0)
    return true;
  return false;
}

bool checkLegalInDirection(char board[][26], int n, int row, int col, char colour, int deltaRow, int deltaCol) {

  char opp = 'W';
  if (colour == 'W')
    opp = 'B';
  
  int k = row + (2 * deltaRow), m = col + (2 * deltaCol);

  if(board[row + deltaRow][col + deltaCol] == opp) {
    while (positionInBounds(n, k, m)) {
      if (board[k][m] == 'U') 
        return false;
      else if (board[k][m] == colour) 
        return true;
      if (deltaRow < 0)
        k--;
      else if (deltaRow > 0)
        k++;
      if (deltaCol < 0)
        m--;
      else if (deltaCol > 0)
        m++; 
    }
  }
  return false;
}

int numTilesFlipped(char board[][26], int size, int row, int col, char colour, int deltaRow, int deltaCol) {

    int tilesFlipped = 0;

    int i = row + deltaRow, j = col + deltaCol;

    while(positionInBounds(size, i, j) && board[i][j] == colour) {
        tilesFlipped++;
        if (deltaRow < 0)
            i--;
        else if (deltaRow > 0)
            i++;
        if (deltaCol < 0)
            j--;
        else if (deltaCol > 0)
            j++; 
    } 

    return tilesFlipped;
}

bool validMove(char board[][26], int size, int row, int col, char colour) {
    
    if(board[row][col] != 'U')
        return false;

    for (int a = -1; a <= 1; a++) {
        for (int b = -1; b <= 1; b++) {
            if(checkLegalInDirection(board, size, row, col, colour, a, b)) 
                return true;
        }
    }
    return false;
}

int playGame(char board[][26], int size, char colour) {

    int indexRow = 0, indexCol = 0, tilesFlipped = 0, score = 0, maxScore = 0;
    const int cornerMultiplier = 100;
    const int edgeMultiplier = 10;
    const int antiEdgeMultiplier = 2;
    char opponent = 'B';

    if(colour == 'B')
        opponent = 'W';
    
    if (!skipTurn(board, size, colour)) {
        
        for(int i = 0; i < size; i++) { 
            for(int j = 0; j < size; j++) {
                //CHECK IF SQUARE IS UNOCCUPIED 
                if(validMove(board, size, i, j, colour)) {
                    //FOR EACH SQUARE CHECKED RESET THE SCORE AND TILES FLIPPED FOR THAT SQUARE
                    score = 0;
                    tilesFlipped = 0; 
                    //CHECK NUMBER OF TILES FLIPPED IN ALL DIRECTIONS
                    for (int a = -1; a <= 1; a++) {
                        for (int b = -1; b <= 1; b++) {
                            if (checkLegalInDirection(board, size, i, j, colour, a, b)) 
                                tilesFlipped += numTilesFlipped(board, size, i, j, opponent, a, b);
                        }
                    }

                    if((i == 0 && j == 0) || (i == size -1 && j == size -1) || (i == 0 && j == size - 1) || (i == size - 1 && j == 0))
                        score = tilesFlipped * cornerMultiplier;
                    else if(i == 0 || i == size - 1 || j == 0 || j == size -1)
                        score = tilesFlipped * edgeMultiplier;
                    else if((i == 1 && j != 0 && j != size -1) || (i == size - 1 && j != 0 && j != size - 1) || (j == 1 && i != 0 && i != size - 1) || (j == size - 1 && i != 0 && i != size -1))
                        score = tilesFlipped / antiEdgeMultiplier;
                    else
                        score = tilesFlipped;

                    if(score > maxScore) {
                        maxScore = score;
                        indexRow = i;
                        indexCol = j;
                    }
                }
            }
        }

        //UPDATE THE BOARD
        board[indexRow][indexCol] = colour;
            
        for (int a = -1; a <= 1; a++) {
            for (int b = -1; b <= 1; b++) {
                if(checkLegalInDirection(board, size, indexRow, indexCol, colour, a, b)) 
                    flipTiles(board, size, indexRow, indexCol, colour, a, b);
            }
        }
    }
    return maxScore;
}

int makeMove(char board[][26], int n, char turn, int *row, int *col) {
    
    int score = 0, totalScore = 0, maxScore = -10000;

    const int checkTime = 1;
    const int cornerMultiplier = 100;
    const int edgeMultiplier = 10;
    const int antiEdgeMultiplier = 2;

    char boardCpy[26][26];
    char currentPlayer = 'B';

    for (int i = 0; i < n; i++) {
        for(int j = 0; j < n; j ++) {

            for(int l = 0; l < n; l++) {
                for(int m = 0; m < n; m++) 
                    boardCpy[l][m] = board[l][m];
            }

            if(validMove(boardCpy, n, i, j, turn)) {
        
                totalScore = 0;
                currentPlayer = 'B';
                if(turn == 'B')
                    currentPlayer = 'W'; 
                
                for (int a = -1; a <= 1; a++) {
                    for (int b = -1; b <= 1; b++) {
                        if(checkLegalInDirection(boardCpy, n, i, j, turn, a, b)) {
                            totalScore += numTilesFlipped(boardCpy, n, i, j, currentPlayer, a, b);
                            boardCpy[i][j] = turn;
                            flipTiles(boardCpy, n, i, j, turn, a, b);
                        }
                    }
                }

                if(((i == 0 && j == 0) || (i == n -1 && j == n -1) || (i == 0 && j == n - 1) || (i == n - 1 && j == 0)))
                    totalScore *= cornerMultiplier;
                else if(i == 0 || i == n - 1 || j == 0 || j == n -1)
                    totalScore *= edgeMultiplier;
                else if((i == 1 && j != 0 && j != n -1) || (i == n - 1 && j != 0 && j != n - 1) || (j == 1 && i != 0 && i != n - 1) || (j == n - 1 && i != 0 && i != n -1))
                    totalScore /= antiEdgeMultiplier;
        
                for(int k = 0; k < checkTime; k++) {
                    score = playGame(boardCpy, n, currentPlayer);
                    if(currentPlayer == turn)
                        totalScore += score;
                    else    
                        totalScore -= score;
                    if(currentPlayer == 'B')
                        currentPlayer = 'W';
                    else    
                        currentPlayer = 'B';
                }

                if(totalScore > maxScore) {
                    maxScore = totalScore;
                    *row = i;
                    *col = j;
                }
            }
        }
    }
    return 0;
}
