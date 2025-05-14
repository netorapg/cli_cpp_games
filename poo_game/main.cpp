#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>

const int WIDTH = 20;
const int HEIGHT = 10;

class Entity {
public:
    int x, y;
    int dx, dy;
    char symbol;

    Entity(int x, int y, char symbol)
        : x(x), y(y), dx(0), dy(0), symbol(symbol) {}

    void move() {
        x += dx;
        y += dy;
        if (x < 0) x = 0;
        if (x >= WIDTH) x = WIDTH - 1;
        if (y < 0) y = 0;
        if (y >= HEIGHT) y = HEIGHT - 1;
    }

    void resetVelocity() {
        dx = 0;
        dy = 0;
    }
};

int kbhit() {
    termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

char getch() {
    termios oldt, newt;
    char ch;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
        newt.c_iflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

class Game {
private:
    Entity player;
public:
    Game() : player(5, 5, '@') {}

    void handleInput() {
        player.resetVelocity();
        if (kbhit()) {
            char key = getch();
            switch (key) {
                case 'w': player.dy = -1; break;
                case 's': player.dy = 1; break;
                case 'a': player.dx = -1; break;
                case 'd': player.dx = 1; break;
            }
        }
    }

    void update() {
        player.move();
    }

    void render() {
        char screen[HEIGHT][WIDTH];
        for (int y = 0; y < HEIGHT; ++y)
            for (int x = 0; x < WIDTH; ++x)
                screen[y][x] = '.';

        screen[player.y][player.x] = player.symbol;

        system("clear");
        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x)
                std::cout << screen[y][x];
            std::cout << '\n';
        }
    }

    void run() {
        while (true) {
            handleInput();
            update();
            render();
            usleep(100000);
        }
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}
