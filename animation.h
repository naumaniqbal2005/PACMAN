#pragma once
#include <SFML/Graphics.hpp>
#include <vector>


enum Direction {
    RIGHT = 0,
    UP = 1,
    DOWN = 2,
    LEFT = 3
};
class Animation {
private:
    std::vector<sf::IntRect> frames;
    std::map<Direction, std::vector<sf::IntRect>> framesPerDirection;
    float durationPerFrame;
    float elapsed;
    size_t currentFrame;

public:
    Animation(float duration = 0.1f)
        : durationPerFrame(duration), elapsed(0.f), currentFrame(0) {
    }

    void addFrame(sf::IntRect rect) {
        frames.push_back(rect);
    }

    void addDirectionalFrame(Direction dir, sf::IntRect rect)
    {
        framesPerDirection[dir].push_back(rect);
    }

    void update(float dt,Direction dir, sf::Sprite& sprite) {
        auto& dirFrames = framesPerDirection[dir];// identifing the frames associated with a particular direction, its a refrence to that particula vector
        if (dirFrames.empty()) return;

        elapsed += dt;
        if (elapsed >= durationPerFrame) {
            elapsed = 0.f;
            currentFrame = (currentFrame + 1) % dirFrames.size();
            sprite.setTextureRect(dirFrames[currentFrame]);
        }
    }

    void updatetele(Direction dir, sf::Sprite& sprite)
    {
        if (frames.empty()) return;

        if (dir < frames.size())
        {
            sprite.setTextureRect(frames[dir]);
        }

    }

    void reset() {
        elapsed = 0.f;
        currentFrame = 0;
    }
};


class Pacman {
private:

    std::map<Direction, sf::Texture> textures;
    sf::Texture texture;
    sf::Sprite sprite;
    Animation animation;
    Direction currentDirection;

public:
    Pacman(const std :: map<Direction, std::string>& spriteSheetPaths, int frameCount, int frameWidth, int frameHeight)
        : animation(0.075f),currentDirection(RIGHT) // 0.1s per frame
    {
        for (const auto& pair : spriteSheetPaths) {
            Direction dir = pair.first;
            std::string path = pair.second;

            sf::Texture tex;
            tex.loadFromFile(path);
            textures[dir] = tex;

            for (int i = 0; i < frameCount; ++i) {
                animation.addDirectionalFrame(dir, sf::IntRect(i * frameWidth, 0, frameWidth, frameHeight));
            }
        }
        sprite.setTexture(textures[RIGHT]);
        sprite.setPosition(100.f, 100.f);

    }

    void setDirection(Direction dir) {
        if (currentDirection != dir) {
            currentDirection = dir;
        }
    }

    void update(float dt) {

        sprite.setTexture(textures[currentDirection]);
        animation.update(dt,currentDirection,sprite);
    }

    void draw(sf::RenderWindow& window) {
        window.draw(sprite);
    }

    void move(Direction dir, float dx, float dy) {
        currentDirection = dir;
        sprite.move(dx, dy);
    }
};


class Teleporter
{
private:
    sf::Texture texture;
    sf::Sprite sprite;
    Animation animation;
public:

    Teleporter(const std::string& teleportersheetpath, int frameCount, int frameWidth, int frameHeight)
        :animation(0.075)
    {
        texture.loadFromFile("TELEPORTER.png");
        sprite.setTexture(texture);
        sprite.setPosition(100.f, 200.f);

        for (int i = 0; i < frameCount; i++)
        {
            animation.addFrame(sf::IntRect(i * frameWidth, 0, frameWidth, frameHeight));
        }
    }

    void update(Direction dir)
    {
        animation.updatetele(dir, sprite);
    }

    void draw(sf::RenderWindow& window) {
        window.draw(sprite);
    }
};