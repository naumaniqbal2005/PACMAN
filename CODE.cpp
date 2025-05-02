#include <SFML/Graphics.hpp>
#include "animation.h"

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "Pac-Man");
    sf::Clock clock;
    float speed = 0.15;
    std::map<Direction, std::string> paths = {
    {RIGHT, "PACMANRIGHT.png"},
    {LEFT, "PACMANLEFT.png"},
    {UP, "PACMANUP.png"},
    {DOWN, "PACMANDOWN.png"}
    };

    Pacman pacman(paths, 4, 50, 50); // 3 frames of 32x32
    Teleporter ghost("TELEPORTER.PNG", 4, 50, 50);
    ghost.update(RIGHT);
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

       

        float dt = clock.restart().asSeconds();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            ghost.update(RIGHT);
            pacman.setDirection(RIGHT);
            pacman.move(RIGHT,speed * 2.f, 0.f);
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
            ghost.update(UP);
            pacman.setDirection(UP);
            pacman.move(UP, 0.f, speed *-2.f);
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
            ghost.update(DOWN);
            pacman.setDirection(DOWN);
            pacman.move(DOWN, 0.f, speed* 2.f);
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            ghost.update(LEFT);
            pacman.setDirection(LEFT);
            pacman.move(LEFT, speed * -2.f, 0.f);
        }
        pacman.update(dt);


        window.clear();
        pacman.draw(window);
        ghost.draw(window);
        window.display();
    }

    return 0;
}
