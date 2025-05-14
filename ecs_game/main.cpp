#include <iostream>
#include <unordered_map>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>

struct Position {
    int x, y;
};

struct Velocity {
    int dx = 0, dy = 0;
};

struct Renderable {
    char symbol;
};

using Entity = int;

class World {
public:
    std::unordered_map<Entity, Position> positions;
    std::unordered_map<Entity, Velocity> velocities;
    std::unordered_map<Entity, Renderable> renderables;

    Entity createEntity() {
        return nextId++;
    }

private:
    Entity nextId = 0;
};

// Constants
const int WIDTH = 20;
const int HEIGHT = 10;

// Lê tecla sem bloqueio no Linux
int kbhit() {
    termios oldt, newt;
    int ch;
    int oldf;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); // desliga modo canônico e eco
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
    struct termios oldt, newt;
    char ch;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); // modo raw
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    ch = getchar();
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

void render(World& world) {
    char screen[HEIGHT][WIDTH];
    // Limpar a tela
    for (int y = 0; y < HEIGHT; ++y)
        for (int x = 0; x < WIDTH; ++x)
            screen[y][x] = '.';

    // Colocar entidades renderizáveis
    for (auto& [id, render] : world.renderables) {
        auto pos = world.positions[id];
        if (pos.x >= 0 && pos.x < WIDTH && pos.y >= 0 && pos.y < HEIGHT)
            screen[pos.y][pos.x] = render.symbol;
    }

    system("clear"); // para Linux/macOS
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            std::cout << screen[y][x];
        }
        std::cout << "\n";
    }
}

void input(World& world, Entity player) {
    world.velocities[player] = {0, 0};
    if (kbhit()) {
        char key = getch();
        if (key == 'w') world.velocities[player].dy = -1;
        if (key == 's') world.velocities[player].dy = 1;
        if (key == 'a') world.velocities[player].dx = -1;
        if (key == 'd') world.velocities[player].dx = 1;
    }
}

void update(World& world) {
    for (auto& [id, vel] : world.velocities) {
        auto& pos = world.positions[id];
        pos.x += vel.dx;
        pos.y += vel.dy;
        // limitar aos limites do mundo
        if (pos.x < 0) pos.x = 0;
        if (pos.y < 0) pos.y = 0;
        if (pos.x >= WIDTH) pos.x = WIDTH - 1;
        if (pos.y >= HEIGHT) pos.y = HEIGHT - 1;
    }
}

int main() {
    World world;

    // Criar player
    Entity player = world.createEntity();
    world.positions[player] = {5, 5};
    world.velocities[player] = {};
    world.renderables[player] = {'@'};

    while (true) {
        input(world, player);
        update(world);
        render(world);
        usleep(100000); // 100ms
    }

    return 0;
}

