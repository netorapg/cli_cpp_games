#include <iostream>
#include <functional>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <memory>
#include <thread>
#include <chrono>

const int WIDTH = 20;
const int HEIGHT = 10;

// === Eventos ===
struct Event {
    virtual ~Event() = default;
};

struct InputEvent : public Event {
    char key;
    InputEvent(char k) : key(k) {}
};

struct MoveEvent : public Event {
    int dx, dy;
    MoveEvent(int dx_, int dy_) : dx(dx_), dy(dy_) {}
};

struct AttackEvent : public Event {
    int x, y;
    AttackEvent(int x_, int y_) : x(x_), y(y_) {}
};

// === EventBus ===
class EventBus {
    std::unordered_map<std::type_index, std::vector<std::function<void(const Event&)>>> listeners;

public:
    template<typename T>
    void subscribe(std::function<void(const T&)> callback) {
        listeners[typeid(T)].push_back([callback](const Event& e) {
            callback(static_cast<const T&>(e));
        });
    }

    void emit(const Event& event) const {
        auto it = listeners.find(typeid(event));
        if (it != listeners.end()) {
            for (auto& listener : it->second) {
                listener(event);
            }
        }
    }
};

// === Player ===
struct Player {
    int x = 5;
    int y = 5;
    char symbol = '@';
};

// Variáveis globais para a espada
bool sword_visible = false;
int sword_x = 0, sword_y = 0;
std::chrono::steady_clock::time_point sword_last_shown;

// === Funções auxiliares para Linux ===
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
    termios new_settings = oldt;
    new_settings.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
    
    ch = getchar();
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

// === Sistemas ===
void InputSystem(EventBus& bus) {
    if (kbhit()) {
        char key = getch();
        bus.emit(InputEvent(key));
    }
}

void InputToMovementSystem(EventBus& bus) {
    bus.subscribe<InputEvent>([&bus](const InputEvent& evt) {
        int dx = 0, dy = 0;
        switch (evt.key) {
            case 'w': dy = -1; break;
            case 's': dy = 1; break;
            case 'a': dx = -1; break;
            case 'd': dx = 1; break;
        }
        if (dx != 0 || dy != 0) {
            bus.emit(MoveEvent(dx, dy));
        }
    });
}

void MovementSystem(Player& player, EventBus& bus) {
    bus.subscribe<MoveEvent>([&player](const MoveEvent& evt) {
        player.x += evt.dx;
        player.y += evt.dy;

        if (player.x < 0) player.x = 0;
        if (player.y < 0) player.y = 0;
        if (player.x >= WIDTH) player.x = WIDTH - 1;
        if (player.y >= HEIGHT) player.y = HEIGHT - 1;

        sword_visible = false; // Esconde a espada ao mover
    });
}

void InputToAttackSystem(EventBus& bus) {
    bus.subscribe<InputEvent>([&bus](const InputEvent& evt) {
        int dx = 0, dy = 0;
        switch (evt.key) {
            case 'i': dy = -1; break;
            case 'k': dy = 1; break;
            case 'j': dx = -1; break;
            case 'l': dx = 1; break;
        }
        if (dx != 0 || dy != 0) {
            bus.emit(AttackEvent(dx, dy));
        }
    });
}

void AttackSystem(Player& player, EventBus& bus) {
    bus.subscribe<AttackEvent>([&player](const AttackEvent& evt) {
        sword_x = player.x + evt.x;
        sword_y = player.y + evt.y;
        
        // Verifica os limites
        if (sword_x < 0) sword_x = 0;
        if (sword_y < 0) sword_y = 0;
        if (sword_x >= WIDTH) sword_x = WIDTH - 1;
        if (sword_y >= HEIGHT) sword_y = HEIGHT - 1;
        
        sword_visible = true;
        sword_last_shown = std::chrono::steady_clock::now(); // Atualiza o tempo da última exibição
    });
}

// === Render ===
void render(const Player& player) {
    char screen[HEIGHT][WIDTH];
    for (int y = 0; y < HEIGHT; ++y)
        for (int x = 0; x < WIDTH; ++x)
            screen[y][x] = '.';

    screen[player.y][player.x] = player.symbol;

    if (sword_visible) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - sword_last_shown).count();
        if (elapsed < 500) { // 0.5 segundos
            screen[sword_y][sword_x] = 'S';
        } else {
            sword_visible = false;
        }
    }

    system("clear"); // Linux
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x)
            std::cout << screen[y][x];
        std::cout << '\n';
    }
}

// === Game loop ===
int main() {
    EventBus bus;
    Player player;

    // Registra todos os sistemas
    InputToMovementSystem(bus);
    MovementSystem(player, bus);
    InputToAttackSystem(bus);
    AttackSystem(player, bus);

    while (true) {
        InputSystem(bus);
        render(player);
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }

    return 0;
}