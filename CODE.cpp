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
    Teleporter ghost1("TELEPORTER.PNG", 4, 50, 50);
    Chaser ghost2("CHASER.png", 4, 50, 50);
    Ambusher ghost3("AMBUSHER.png", 4, 50, 50);
    Random ghost4("RANDOM.png", 4, 50, 50);
    Hermes ghost5("HERMES.png", 4, 50, 50);
    TimeStop ghost6("TIMESTOP.png", 4, 50, 50);
    Phantom ghost7("PHANTOM.png", 4, 50, 50);
    RingGhost ghost8("RINGGHOST.png", 4, 50, 50);
    ghost1.update(RIGHT);
    ghost2.update(RIGHT);
    ghost3.update(RIGHT);
    ghost4.update(RIGHT);
    ghost5.update(RIGHT);
    ghost6.update(RIGHT);
    ghost7.update(RIGHT);
    ghost8.update(RIGHT);
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

       

        float dt = clock.restart().asSeconds();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            ghost1.update(RIGHT);
            ghost2.update(RIGHT);
            ghost3.update(RIGHT);
            ghost4.update(RIGHT);
            ghost5.update(RIGHT);
            ghost6.update(RIGHT);
            ghost7.update(RIGHT);
            ghost8.update(RIGHT);
            
            pacman.setDirection(RIGHT);
            pacman.move(RIGHT,speed * 2.f, 0.f);
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
            ghost1.update(UP);
            ghost2.update(UP);
            ghost3.update(UP);
            ghost4.update(UP);
            ghost5.update(UP);
            ghost6.update(UP);
            ghost7.update(UP);
            ghost8.update(UP);
            pacman.setDirection(UP);
            pacman.move(UP, 0.f, speed *-2.f);
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
            ghost1.update(DOWN);
            ghost2.update(DOWN);
            ghost3.update(DOWN);
            ghost4.update(DOWN);
            ghost5.update(DOWN);
            ghost6.update(DOWN);
            ghost7.update(DOWN);
            ghost8.update(DOWN);
            pacman.setDirection(DOWN);
            pacman.move(DOWN, 0.f, speed* 2.f);
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            ghost1.update(LEFT);
            ghost2.update(LEFT);
            ghost3.update(LEFT);
            ghost4.update(LEFT);
            ghost5.update(LEFT);
            ghost6.update(LEFT);
            ghost7.update(LEFT);
            ghost8.update(LEFT);
            pacman.setDirection(LEFT);
            pacman.move(LEFT, speed * -2.f, 0.f);
        }
        pacman.update(dt);


        window.clear();
        pacman.draw(window);
        ghost1.draw(window);
        ghost2.draw(window);
        ghost3.draw(window);
        ghost4.draw(window);
        ghost5.draw(window);
        ghost6.draw(window);
        ghost7.draw(window);
        ghost8.draw(window);
        window.display();
    }

    return 0;
}
