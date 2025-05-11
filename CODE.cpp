#include <SFML/Graphics.hpp>
#include "maze.h"
#include "pacman.h"
#include "Ghosts.h"
#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <map>
#include <string>
#include <algorithm>
#include <random>
#include <iostream>

using namespace std;
using namespace sf;

const int windowWidth = 960;
const int windowHeight = 1050;

vector<string> ghostNames = {
    "TELEPORTER", "RANDOMGHOST", "CHASER", "AMBUSHER",
    "HERMES", "PHANTOM", "TIMESTOP", "RINGGHOST"
};

struct Dot {
    CircleShape shape;
    float speed;
};

void generateBackgroundDots(vector<Dot>& dots) {
    for (int i = 0; i < 100; ++i) {
        CircleShape dot(2);
        dot.setFillColor(Color::White);
        dot.setPosition(rand() % windowWidth, rand() % windowHeight);
        float speed = 1.f + static_cast<float>(rand() % 100) / 100.f;
        dots.push_back({ dot, speed });
    }
}

vector<Ghost*> createMenuGhosts() {
    random_device rd;
    mt19937 gen(rd());
    shuffle(ghostNames.begin(), ghostNames.end(), gen);
    vector<Ghost*> ghosts;

    // Ensure that ghostNames contains at least 4 elements.
    if (ghostNames.size() < 4) {
        cerr << "Not enough ghost names to create menu ghosts!" << endl;
        return ghosts;  // Return empty vector if there aren't enough names.
    }

    for (int i = 0; i < 4; ++i) {
        string ghostName = ghostNames[i];
        string spriteSheetPath = ghostName + ".png";

        map<Direction, int> frameIndexes = {
            {RIGHT, 0}, {UP, 1}, {DOWN, 2}, {LEFT, 3}
        };

        float x = -120.0f * (i + 1);
        float y = 700.f;

        Ghost* g = new Ghost(spriteSheetPath, 4, 50, 50, x, y, 2.0f, 5.0f, frameIndexes);
        ghosts.push_back(g);
    }

    return ghosts;
}

void updateDots(vector<Dot>& dots) {
    for (auto& d : dots) {
        Vector2f pos = d.shape.getPosition();
        pos.y += d.speed;
        if (pos.y > windowHeight) pos.y = 0;
        d.shape.setPosition(pos);
    }
}

void spawnGameGhosts(vector<Ghost*>& gameGhosts, const Maze& maze) {
    vector<string> ghostNames = {
        "RANDOMGHOST", "CHASER", "AMBUSHER", "PHANTOM",
        "HERMES", "RINGGHOST"
    };

    random_device rd;
    mt19937 gen(rd());
    shuffle(ghostNames.begin(), ghostNames.end(), gen);

    for (int i = 0; i < 4; ++i) {
        string ghostName = ghostNames[i];
        string spriteSheetPath = ghostName + ".png";

        map<Direction, int> frameIndexes = {
            {RIGHT, 0}, {UP, 1}, {DOWN, 2}, {LEFT, 3}
        };

        Vector2i ghostPos = maze.getGhost(i + '0');

        if (ghostPos.x == -1 || ghostPos.y == -1) continue;

        static const int TILE_SIZE = 40;
        float x = ghostPos.x * TILE_SIZE;
        float y = ghostPos.y * TILE_SIZE;

        Ghost* g = new Ghost(spriteSheetPath, 4, 50, 50,
            x + 35, y + 35, 2.0f, 1.0f,
            frameIndexes); // Removed GhostType argument
        gameGhosts.push_back(g);
    }
}

void drawMenu(RenderWindow& window, Text& title, vector<Text>& menuTexts, int selectedItem,
    vector<Ghost*>& menuGhosts, vector<Dot>& dots, bool inMenu) {

    for (auto& d : dots)
        window.draw(d.shape);

    window.draw(title);

    for (size_t i = 0; i < menuTexts.size(); ++i) {
        menuTexts[i].setFillColor(i == selectedItem ? Color::Yellow : Color::White);
        window.draw(menuTexts[i]);
    }

    if (inMenu) {
        // Define initial positions for the menu ghosts
        static const float initialPositions[4] = { -120.f, -120.f, -120.f, -120.f };

        for (int i = 0; i < 4; ++i) {
            Ghost* g = menuGhosts[i];

            // Move the ghost in the RIGHT direction
            g->menMove(RIGHT);

            // Update ghost's movement with fixed delta time
            g->Update(1.0f / 60.0f);

            if (g->GetPosition().x > windowWidth) {
                // Calculate new x-coordinate based on ghost index to maintain consistent spacing.
                float newX = -190.f;  // Reset to initial position with a consistent step.
                g->SetPosition(newX, 700.f);  // Reset y-coordinate if needed.
            }

            window.draw(g->getSprite());
        }
    }
}

void MainGame() {
    RenderWindow window(VideoMode(windowWidth, windowHeight), "Pac-Man");
    window.setFramerateLimit(60);

    Font font;
    if (!font.loadFromFile("ArcadeClassic.ttf")) {
        cerr << "Error: Could not load font ArcadeClassic.ttf" << endl;
        return;
    }

    Text title("PAC-MAN", font, 80);
    title.setFillColor(Color::Yellow);
    title.setPosition(windowWidth / 2.f - title.getGlobalBounds().width / 2.f, 200);

    vector<string> menuItems = { "Start Game", "Options", "Exit" };
    vector<Text> menuTexts;
    int selectedItem = 0;

    for (size_t i = 0; i < menuItems.size(); ++i) {
        Text item(menuItems[i], font, 40);
        item.setFillColor(Color::White);
        item.setPosition(windowWidth / 2.f - item.getGlobalBounds().width / 2.f, 320 + i * 90);
        menuTexts.push_back(item);
    }

    vector<Dot> dots;
    generateBackgroundDots(dots);

    Maze maze;
    Vector2i pacmanCell = maze.getP();
    Vector2f offset = maze.getOffset();
    float cellSize = Maze::getCellSize();
    Vector2f pacmanStartPos(pacmanCell.x * cellSize + offset.x, pacmanCell.y * cellSize + offset.y);

    map<Direction, string> pacPaths = {
        { UP, "PACMANUP.png" },
        { DOWN, "PACMANDOWN.png" },
        { LEFT, "PACMANLEFT.png" },
        { RIGHT, "PACMANRIGHT.png" }
    };

    Pacman pacman(pacPaths, 4, 50, 50, pacmanStartPos.x, pacmanStartPos.y, 2.0f);

    vector<Ghost*> menuGhosts = createMenuGhosts();
    vector<Ghost*> gameGhosts;

    bool inMenu = true;
    bool gameStarted = false;

    Clock clock;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed)
                window.close();

            if (event.type == Event::KeyPressed) {
                if (inMenu) {
                    if (event.key.code == Keyboard::Up)
                        selectedItem = (selectedItem - 1 + menuItems.size()) % menuItems.size();
                    else if (event.key.code == Keyboard::Down)
                        selectedItem = (selectedItem + 1) % menuItems.size();
                    else if (event.key.code == Keyboard::Enter || event.key.code == Keyboard::Return) {
                        if (selectedItem == 0) {
                            inMenu = false;
                            gameStarted = true;

                            spawnGameGhosts(gameGhosts, maze);
                            cout << "Game ghosts spawned: " << gameGhosts.size() << endl;
                        }
                        else if (selectedItem == 2) {
                            window.close();
                        }
                    }
                }
                else if (gameStarted) {
                    if (event.key.code == Keyboard::Up)    pacman.SetDirection(UP);
                    if (event.key.code == Keyboard::Down)  pacman.SetDirection(DOWN);
                    if (event.key.code == Keyboard::Left)  pacman.SetDirection(LEFT);
                    if (event.key.code == Keyboard::Right) pacman.SetDirection(RIGHT);
                }
            }
        }

        window.clear(Color::Black);
        updateDots(dots);

        if (inMenu) {
            drawMenu(window, title, menuTexts, selectedItem, menuGhosts, dots, inMenu);
        }
        else if (gameStarted) {
            Vector2f nextPos = pacman.GetPosition();
            float speed = 2.0f;

            switch (pacman.GetDirection()) {
            case UP:    nextPos.y -= speed; break;
            case DOWN:  nextPos.y += speed; break;
            case LEFT:  nextPos.x -= speed; break;
            case RIGHT: nextPos.x += speed; break;
            }

            if (maze.isWalkable(nextPos))
                pacman.Move(pacman.GetDirection(), maze);
            else if (maze.isWall(pacman.GetPosition()))
                pacman.Stop(pacman.GetDirection());

            maze.isFood(pacman.GetPosition());
            maze.isSuperFood(pacman.GetPosition());

            pacman.Update();

            maze.draw(window);
            window.draw(pacman.getSprite());
            for (auto g : gameGhosts) {
                g->updateAutonomous(maze);
                g->Update(dt);

                if (g->GhostCollision(pacman.GetPosition())) {
                    cout << "Pacman collided with a ghost!" << endl;
                }
                window.draw(g->getSprite());
            }

        }

        window.display();
    }

    for (auto g : menuGhosts) delete g;
    for (auto g : gameGhosts) delete g;
}

int main() {
    MainGame();  // Run the main game loop
    return 0;
}
