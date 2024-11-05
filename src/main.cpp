#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono>

class TicTacToe {
public:
    TicTacToe() : board(3, std::vector<char>(3, ' ')), currentPlayer('X'), gameEnded(false) {}

    void exibir_tabuleiro(char player, bool isFinal = false) {
        if (isFinal) {
            std::cout << "\nResultado Final:\n";
        } else {
            #ifdef _WIN32
                system("cls");
            #else
                system("clear");
            #endif
            std::cout << "Current Board:\n";
        }

        for (const auto& row : board) {
            for (const auto& cell : row) {
                std::cout << (cell == ' ' ? '.' : cell) << " ";
            }
            std::cout << "\n";
        }
        std::cout << std::endl;

        std::cout << "Current player: " << player << "\n\n";

    }

    bool fazer_jogada(char player, int row, int col) {
        std::lock_guard<std::mutex> lock(boardMutex);
        if (row < 0 || row >= 3 || col < 0 || col >= 3 || board[row][col] != ' ') {
            return false;
        }

        board[row][col] = player;
        exibir_tabuleiro(player);

        if (checar_vitoria(player)) {
            std::cout << "Player " << player << " wins!\n";
            gameEnded = true;
            turnCv.notify_all();
            return true;
        }

        if (isBoardFull()) {
            std::cout << "Game ended in a draw.\n";
            gameEnded = true;
            turnCv.notify_all();
        }

        togglePlayer();
        return false;
    }

    bool checar_vitoria(char player) {
        for (int i = 0; i < 3; ++i) {
            if ((board[i][0] == player && board[i][1] == player && board[i][2] == player) ||
                (board[0][i] == player && board[1][i] == player && board[2][i] == player)) {
                return true;
            }
        }
        if ((board[0][0] == player && board[1][1] == player && board[2][2] == player) ||
            (board[0][2] == player && board[1][1] == player && board[2][0] == player)) {
            return true;
        }
        return false;
    }

    bool isBoardFull() {
        for (const auto& row : board) {
            for (const auto& cell : row) {
                if (cell == ' ') {
                    return false;
                }
            }
        }
        return true;
    }

    bool is_game_over() {
        std::lock_guard<std::mutex> lock(boardMutex);
        return gameEnded;
    }

    void togglePlayer() {
        currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
        turnCv.notify_all();
    }

    char getCurrentPlayer() const { return currentPlayer; }

    void waitForTurn(char player) {
        std::unique_lock<std::mutex> lock(turnMutex);
        turnCv.wait(lock, [&] { return currentPlayer == player || gameEnded; });
    }

private:
    std::vector<std::vector<char>> board;
    char currentPlayer;
    bool gameEnded;
    mutable std::mutex boardMutex;
    std::mutex turnMutex;
    std::condition_variable turnCv;
};

class Player {
public:
    Player(TicTacToe& game, char symbol) : game(game), symbol(symbol) {}

    void playSequential() {
        for (int i = 0; i < 3 && !game.is_game_over(); ++i) {
            for (int j = 0; j < 3 && !game.is_game_over(); ++j) {
                game.waitForTurn(symbol);
                if (game.is_game_over()) return;

                if (game.fazer_jogada(symbol, i, j)) return;
                std::this_thread::sleep_for(std::chrono::milliseconds(750)); 
            }
        }
    }

    void playRandom() {
        std::srand(std::time(nullptr));
        while (!game.is_game_over()) {
            game.waitForTurn(symbol);
            if (game.is_game_over()) return;

            int row = std::rand() % 3;
            int col = std::rand() % 3;

            if (game.fazer_jogada(symbol, row, col)) return;
            std::this_thread::sleep_for(std::chrono::milliseconds(750)); 
        }
    }

private:
    TicTacToe& game;
    char symbol;
};

int main() {
    TicTacToe game;
    Player playerX(game, 'X');
    Player playerO(game, 'O');

    std::thread t1(&Player::playSequential, &playerX);
    std::thread t2(&Player::playRandom, &playerO);

    t1.join();
    t2.join();

    if (!game.is_game_over()) {
        game.exibir_tabuleiro(game.getCurrentPlayer(), true);
    }

    return 0;
}
