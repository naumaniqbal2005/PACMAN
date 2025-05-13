#include "entity.h"
#include "animation.h"
#include "maze.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>

class Ghost : public Entity {
protected:
    sf::Texture texture;
    Animation animation;
    Direction currentDirection;
    sf::Vector2f initialPosition;
    int frameWidth, frameHeight;
    float behaviorTimer;     // Timer for ghost behaviors
    bool isScattered;        // For alternating between scatter and random movement
    float scatterTimer;      // For timing scatter/random phases
    const float SCATTER_DURATION = 7.0f;  // Seconds in scatter mode
    sf::Color originalColor; // Store the original color for restoration after super mode

    // Constants for cell-based movement
    static const int CELL_SIZE = 40;

public:
    Ghost(const std::string& spriteSheetPath,
        int frameCount, int frameWidth, int frameHeight,
        float x, float y, float speed, float scale,
        const std::map<Direction, int>& frameIndexes)
        : Entity(x, y, speed),
        animation(0.1f),
        currentDirection(LEFT),
        initialPosition(x, y),
        frameWidth(frameWidth),
        frameHeight(frameHeight),
        behaviorTimer(0.0f),
        isScattered(true),
        scatterTimer(0.0f)
    {
        if (!texture.loadFromFile(spriteSheetPath)) {
            std::cerr << "Failed to load ghost texture: " << spriteSheetPath << std::endl;
        }

        sprite.setTexture(texture);
        sprite.setScale(scale, scale);
        sprite.setPosition(position);
        originalColor = sprite.getColor(); // Store the original color

        for (int i = 0; i < frameCount; ++i) {
            animation.addFrame(sf::IntRect(i * frameWidth, 0, frameWidth, frameHeight)); // horizontal layout
        }

        std::srand(static_cast<unsigned int>(std::time(nullptr)));
    }
    void setSpeed(float newSpeed) { speed = newSpeed; }
    float getOriginalSpeed() const { return speed; }
    virtual ~Ghost() = default;

    virtual bool Move(Direction dir, Maze& maze) {
        Vector2f tempPosition = position;
        float moveDist = speed;

        switch (dir) {
        case RIGHT: tempPosition.x += moveDist; break;
        case LEFT:  tempPosition.x -= moveDist; break;
        case UP:    tempPosition.y -= moveDist; break;
        case DOWN:  tempPosition.y += moveDist; break;
        }

        float mazeWidth = Maze::getWidth() * Maze::getCellSize() + maze.getOffset().x;
        if (tempPosition.x < maze.getOffset().x) {
            tempPosition.x = mazeWidth - sprite.getGlobalBounds().width - 10;
        }
        else if (tempPosition.x + sprite.getGlobalBounds().width > mazeWidth-10) {
            tempPosition.x = maze.getOffset().x + 10;
        }

        if (isValidDirection(maze, dir)) {
            position = tempPosition;
            currentDirection = dir;
            sprite.setPosition(position);
            return true;  // Move succeeded
        }

        return false;  // Blocked by wall
    }

    void menMove(Direction dir) {
        currentDirection = dir;

        switch (dir) {
        case LEFT:  position.x -= speed; break;
        case RIGHT: position.x += speed; break;
        case UP:    position.y -= speed; break;
        case DOWN:  position.y += speed; break;
        }

        sprite.setPosition(position);
        Update(1.0f / 60.0f);
    }

    virtual void Update(float deltaTime) {
        animation.updateGhost(currentDirection, sprite);
    }

    virtual void Reset() {
        position = initialPosition;
        sprite.setPosition(position);
        currentDirection = LEFT;
        isScattered = true;
        scatterTimer = 0.0f;
        behaviorTimer = 0.0f;
        animation.reset();
        sprite.setColor(originalColor); // Reset to original color
    }

    sf::Vector2f GetPosition() const {
        return position;
    }

    void SetPosition(float x, float y) {
        position.x = x;
        position.y = y;
        sprite.setPosition(position);
    }

    virtual sf::Sprite getSprite() const {
        return sprite;
    }

    virtual bool GhostCollision(const sf::Vector2f& pacmanPosition) const {
        sf::FloatRect pacmanBounds(
            pacmanPosition.x - 20.f, // center the 40x40 box around pacmanPosition
            pacmanPosition.y - 20.f,
            40.f,
            40.f
        );

        return sprite.getGlobalBounds().intersects(pacmanBounds);
    }

    virtual void updateAutonomous(Maze& maze) {
        float deltaTime = 1.0f / 60.0f;

        // Try to move in current direction
        if (!Move(currentDirection, maze)) {
            // If blocked, pick a new valid direction (excluding opposite)
            std::vector<Direction> possibleDirs;

            for (int d = 0; d < 4; ++d) {
                Direction dir = static_cast<Direction>(d);
                if (dir == getOpposite(currentDirection)) continue;

                if (isValidDirection(maze, dir)) {
                    possibleDirs.push_back(dir);
                }
            }

            if (!possibleDirs.empty()) {
                currentDirection = possibleDirs[std::rand() % possibleDirs.size()];
                Move(currentDirection, maze);  // Try the new direction immediately
            }
        }

        Update(deltaTime);
    }

    Direction GetCurrentDirection() const {
        return currentDirection;
    }

    std::vector<Direction> getAvailableDirections(Maze& maze) {
        std::vector<Direction> dirs;
        Vector2f testPos;

        // Try each direction, excluding the opposite of current direction
        for (int d = 0; d < 4; ++d) {
            Direction dir = static_cast<Direction>(d);

            // Skip the opposite direction (no backtracking)
            if (dir == getOpposite(currentDirection)) {
                continue;
            }

            testPos = position;

            // Check the position for each direction
            switch (dir) {
            case UP:    testPos.y -= CELL_SIZE; break;
            case DOWN:  testPos.y += CELL_SIZE; break;
            case LEFT:  testPos.x -= CELL_SIZE; break;
            case RIGHT: testPos.x += CELL_SIZE; break;
            }

            // If the new position is walkable, add this direction to the list
            if (maze.isWalkable(testPos)) {
                dirs.push_back(dir);
            }
        }

        // If no directions are valid (i.e., dead-end), go back
        if (dirs.empty()) {
            dirs.push_back(getOpposite(currentDirection));
        }

        return dirs;
    }

    Direction getOpposite(Direction dir) const {
        switch (dir) {
        case UP: return DOWN;
        case DOWN: return UP;
        case LEFT: return RIGHT;
        case RIGHT: return LEFT;
        default: return LEFT;
        }
    }

    bool isAwayFromCenterEnough() const {
        float minDistance = 25.0f;  // Ghost must be at least this far from cell center

        float centerX = std::round(position.x / CELL_SIZE) * CELL_SIZE + (CELL_SIZE / 2);
        float centerY = std::round(position.y / CELL_SIZE) * CELL_SIZE + (CELL_SIZE / 2);

        float dx = std::abs(position.x - centerX);
        float dy = std::abs(position.y - centerY);

        return dx > minDistance || dy > minDistance;
    }

    bool isValidDirection(Maze& maze, Direction dir) {
        Vector2f testPos = position;

        // Round to center of current cell
        testPos.x = std::floor(testPos.x / CELL_SIZE) * CELL_SIZE + CELL_SIZE / 2;
        testPos.y = std::floor(testPos.y / CELL_SIZE) * CELL_SIZE + CELL_SIZE / 2;

        // Move one full cell in the given direction
        switch (dir) {
        case RIGHT: testPos.x += CELL_SIZE; break;
        case LEFT:  testPos.x -= CELL_SIZE; break;
        case UP:    testPos.y -= CELL_SIZE; break;
        case DOWN:  testPos.y += CELL_SIZE; break;
        }

        return maze.isWalkable(testPos);
    }

    void SuperSpeed() {
        speed += 1.f;  // Increase speed   
    }

    void ResetSpeed() {
        speed -= 1.f;  // Reset speed
    }

    // Set the color of the ghost for super mode
    void setColor(const sf::Color& color) {
        sprite.setColor(color);
    }

    // Get the original color of the ghost
    sf::Color getOriginalColor() const {
        return originalColor;
    }
};

class RingGhost : public Ghost {
private:
    bool isVisible;
    float visibilityTimer;
    const float INVISIBLE_DURATION = 3.0f;
    const float VISIBLE_DURATION = 5.0f;
    const float BLINK_WARNING_DURATION = 1.0f;  // Duration of blinking warning before state change
    bool isBlinking;  // Flag to indicate if ghost is currently blinking
    float blinkTimer;  // Timer for controlling blink frequency

public:
    RingGhost(const std::string& spriteSheetPath,
        int frameCount, int frameWidth, int frameHeight,
        float x, float y, float speed, float scale,
        const std::map<Direction, int>& frameIndexes)
        : Ghost(spriteSheetPath, frameCount, frameWidth, frameHeight, x, y, speed, scale, frameIndexes),
        isVisible(true),
        visibilityTimer(0.0f),
        isBlinking(false),
        blinkTimer(0.0f) {
        updateVisibility();
    }

    bool getIsVisible() const { return isVisible; }
    float getVisibilityTimer() const { return visibilityTimer; }
    bool getIsBlinking() const { return isBlinking; }

    void Update(float deltaTime) override {
        Ghost::Update(deltaTime);
        visibilityTimer += deltaTime;

        // Check if we need to start blinking before state change
        if (isVisible && !isBlinking && visibilityTimer >= VISIBLE_DURATION - BLINK_WARNING_DURATION) {
            isBlinking = true;
            blinkTimer = 0.0f;
        }
        else if (!isVisible && !isBlinking && visibilityTimer >= INVISIBLE_DURATION - BLINK_WARNING_DURATION) {
            isBlinking = true;
            blinkTimer = 0.0f;
        }

        // Handle blinking effect
        if (isBlinking) {
            blinkTimer += deltaTime;

            // Toggle visibility rapidly (10 times per second)
            sf::Color color = sprite.getColor();
            if (static_cast<int>(blinkTimer * 10) % 2 == 0) {
                color.a = 255;  // Fully visible
            }
            else {
                color.a = 80;   // Semi-transparent
            }
            setColor(color);

            // Check if blinking period is over and we need to change state
            if ((isVisible && visibilityTimer >= VISIBLE_DURATION) ||
                (!isVisible && visibilityTimer >= INVISIBLE_DURATION)) {
                // Change visibility state
                isVisible = !isVisible;
                isBlinking = false;
                visibilityTimer = 0.0f;
                updateVisibility();
            }
        }
    }

    void updateVisibility() {
        sf::Color color = sprite.getColor();
        color.a = isVisible ? 255 : 0; // Full opacity when visible, transparent when invisible
        setColor(color);
    }

    // Override the getSprite method to control visibility
    sf::Sprite getSprite() const override {
        return Ghost::getSprite();
    }

    // RingGhost can still collide even when invisible
    bool GhostCollision(const sf::Vector2f& pacmanPosition) const override {
        // Ghost can collide even when invisible
        return Ghost::GhostCollision(pacmanPosition);
    }

    void Reset() override {
        Ghost::Reset();
        isVisible = true;
        visibilityTimer = 0.0f;
        isBlinking = false;
        blinkTimer = 0.0f;
        updateVisibility();
    }
};

class TeleporterGhost : public Ghost {
private:
    float teleportTimer;            // Track time until next teleport
    const float TELEPORT_INTERVAL = 10.0f;  // Seconds between teleports
    const float FLICKER_DURATION = 2.0f;    // Duration of flickering before teleport
    bool isFlickering;              // Flag for flickering state
    float flickerTimer;             // Timer for controlling flicker frequency
    std::vector<sf::Vector2f> teleportLocations; // Predefined teleport locations

public:
    TeleporterGhost(const std::string& spriteSheetPath,
        int frameCount, int frameWidth, int frameHeight,
        float x, float y, float speed, float scale,
        const std::map<Direction, int>& frameIndexes)
        : Ghost(spriteSheetPath, frameCount, frameWidth, frameHeight, x, y, speed, scale, frameIndexes),
        teleportTimer(0.0f),
        isFlickering(false),
        flickerTimer(0.0f)
    {
        // Initialize predefined teleport locations (adjust coordinates based on your maze)
        initTeleportLocations();
    }

    void initTeleportLocations() {
        // Define 8 valid teleport locations based on the provided grid coordinates
        // Converting grid positions to pixel positions with CELL_SIZE = 40
        // For each grid position (x,y), calculate pixel position as (x*40 + offset.x, y*40 + offset.y)
        // to place the ghost at the correct position accounting for maze offset

        const sf::Vector2f offset(60.f, 40.f); // Maze offset

        const std::vector<sf::Vector2i> TELEPORT_POINTS = {
            {2, 1},   // Top-left corner
            {17, 1},  // Top-right
            {2, 15},  // Bottom-left
            {17, 15}, // Bottom-right
            {9, 4},   // Center-top
            {9, 12},  // Center-bottom
            {4, 9},   // Left-center
            {14, 9}   // Right-center
        };

        teleportLocations.clear();
        // Convert grid coordinates to pixel coordinates with offset
        for (const auto& point : TELEPORT_POINTS) {
            // Calculate center of the tile with offset
            float x = point.x * CELL_SIZE + offset.x;
            float y = point.y * CELL_SIZE + offset.y;
            teleportLocations.push_back(sf::Vector2f(x, y));
        }
    }

void Update(float deltaTime) override {
    Ghost::Update(deltaTime);
    teleportTimer += deltaTime;

    // Check if it's time to start flickering before teleport
    if (!isFlickering && teleportTimer >= TELEPORT_INTERVAL - FLICKER_DURATION) {
        isFlickering = true;
        flickerTimer = 0.0f;
    }

    // Handle flickering effect
    if (isFlickering) {
        flickerTimer += deltaTime;

        // Toggle visibility rapidly (15 times per second)
        sf::Color color = sprite.getColor();
        if (static_cast<int>(flickerTimer * 15) % 2 == 0) {
            color.a = 255;  // Fully visible
        }
        else {
            color.a = 100;  // Semi-transparent
        }
        setColor(color);

        // Check if flickering period is over and we need to teleport
        if (teleportTimer >= TELEPORT_INTERVAL) {
            teleport();
            isFlickering = false;
            teleportTimer = 0.0f;

            // Reset color to fully visible after teleport
            sf::Color fullColor = sprite.getColor();
            fullColor.a = 255;
            setColor(fullColor);
        }
    }
}

void teleport() {
    // Choose a random location from predefined teleport points
    int randomIndex = std::rand() % teleportLocations.size();
    sf::Vector2f newPos = teleportLocations[randomIndex];

    // Teleport the ghost
    SetPosition(newPos.x, newPos.y);

    // Create a teleport effect (optional)
    // You could add particle effects or sound here
}

bool isTeleportValid(const sf::Vector2f& position, Maze& maze) const {
    // Check if the position is walkable in the maze
    // This method can be used to verify teleport positions
    return maze.isWalkable(position);
}

// Add a method to dynamically find valid teleport positions from the maze
void updateTeleportLocations(Maze& maze) {
    teleportLocations.clear();

    // Scan the maze for walkable areas and add them as potential teleport locations
    // This is an alternative to hardcoded positions
    for (int y = 1; y < maze.getHeight() - 1; y += 4) {  // Skip every few rows for diversity
        for (int x = 1; x < maze.getWidth() - 1; x += 4) { // Skip every few columns
            sf::Vector2f pos(x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2);
            if (maze.isWalkable(pos)) {
                teleportLocations.push_back(pos);

                // Limit to 8 positions maximum
                if (teleportLocations.size() >= 8) {
                    return;
                }
            }
        }
    }

    // Fallback if we found fewer than 8 positions
    if (teleportLocations.empty()) {
        // Add a default position as fallback
        teleportLocations.push_back(sf::Vector2f(180.f, 180.f));
    }
}

void Reset() override {
    Ghost::Reset();
    teleportTimer = 0.0f;
    isFlickering = false;
    flickerTimer = 0.0f;

    // Ensure full visibility
    sf::Color color = sprite.getColor();
    color.a = 255;
    setColor(color);
}

// The following methods are inherited and used as-is:
// updateAutonomous, GhostCollision, etc.

// Optional: make the teleporter ghost move more aggressively
void updateAutonomous(Maze& maze) override {
    float deltaTime = 1.0f / 60.0f;

    // Try to move in current direction
    if (!Move(currentDirection, maze)) {
        // If blocked, pick a new valid direction (excluding opposite)
        std::vector<Direction> possibleDirs;

        for (int d = 0; d < 4; ++d) {
            Direction dir = static_cast<Direction>(d);
            if (dir == getOpposite(currentDirection)) continue;

            if (isValidDirection(maze, dir)) {
                possibleDirs.push_back(dir);
            }
        }

        if (!possibleDirs.empty()) {
            currentDirection = possibleDirs[std::rand() % possibleDirs.size()];
            Move(currentDirection, maze);  // Try the new direction immediately
        }
    }

    Update(deltaTime);
}
};

class PhantomGhost : public Ghost
{
    float EXTENDED_RADIUS;
public:
    PhantomGhost(const std::string& spriteSheetPath,
        int frameCount, int frameWidth, int frameHeight,
        float x, float y, float speed, float scale,
        const std::map<Direction, int>& frameIndexes)
        : Ghost(spriteSheetPath, frameCount, frameWidth, frameHeight, x, y, speed, scale, frameIndexes), EXTENDED_RADIUS(2.0f) {
    }
        bool GhostCollision(const sf::Vector2f& pacmanPosition) const override {
        // Create larger bounds for collision detection
        sf::FloatRect pacmanBounds(
            pacmanPosition.x - 20.f * EXTENDED_RADIUS,
            pacmanPosition.y - 20.f * EXTENDED_RADIUS,
            40.f * EXTENDED_RADIUS,
            40.f * EXTENDED_RADIUS
        );

        return getSprite().getGlobalBounds().intersects(pacmanBounds);
    }
};

class AmbusherGhost : public Ghost {
private:
    bool isPaused;
    float pauseTimer;
    const float PAUSE_DURATION = 3.0f;  // 2 seconds pause on 'o' tiles

    // Hardcoded 'o' positions from the map (in grid coordinates)
    std::vector<sf::Vector2i> pausePositions;

    // Maze offset
    sf::Vector2f mazeOffset;

    // Flag to ensure we only pause once on each 'o' tile
    sf::Vector2i lastPauseTile;
    bool hasPausedOnCurrentTile;

public:
    AmbusherGhost(const std::string& spriteSheetPath,
        int frameCount, int frameWidth, int frameHeight,
        float x, float y, float speed, float scale,
        const std::map<Direction, int>& frameIndexes)
        : Ghost(spriteSheetPath, frameCount, frameWidth, frameHeight, x, y, speed, scale, frameIndexes),
        isPaused(false),
        pauseTimer(0.0f),
        mazeOffset(60.f, 40.f),  // Setting the maze offset
        lastPauseTile(-1, -1),
        hasPausedOnCurrentTile(false)
    {
        // Initialize the hardcoded 'o' positions from the map data
        // Format is {x, y} in grid coordinates (not pixels)
        pausePositions = {
            {2, 2},    // Top left o
            {17, 2},   // Top right o
            {2, 15},   // Bottom left o
            {17, 15}   // Bottom right o
        };
    }

    void Update(float deltaTime) override {
        // Call the base class animation update
        Ghost::Update(deltaTime);

        // If paused, update the timer
        if (isPaused) {
            pauseTimer += deltaTime;

            // Resume movement if pause duration has elapsed
            if (pauseTimer >= PAUSE_DURATION) {
                isPaused = false;
                pauseTimer = 0.0f;

                // Force a small movement to ensure we break out of the pause state
                // Move slightly in current direction
                switch (GetCurrentDirection()) {
                case UP:    position.y -= 2.0f; break;
                case DOWN:  position.y += 2.0f; break;
                case LEFT:  position.x -= 2.0f; break;
                case RIGHT: position.x += 2.0f; break;
                }
                sprite.setPosition(position);

                // Print debug info
                std::cout << "Ghost resumed movement at position: ("
                    << position.x << ", " << position.y << ")" << std::endl;
            }
        }
    }

    void updateAutonomous(Maze& maze) override {
        // If paused, just update the timer but don't move
        if (isPaused) {
            Update(1.0f / 60.0f);
            return;
        }

        // Get current cell position, accounting for maze offset
        int cellX = static_cast<int>((position.x - mazeOffset.x) / CELL_SIZE);
        int cellY = static_cast<int>((position.y - mazeOffset.y) / CELL_SIZE);

        // If we've moved to a different cell, reset the pause flag for this cell
        if (cellX != lastPauseTile.x || cellY != lastPauseTile.y) {
            hasPausedOnCurrentTile = false;
        }

        // Calculate center of cell in pixel coordinates, accounting for maze offset
        float centerX = (cellX * CELL_SIZE) + (CELL_SIZE / 2) + mazeOffset.x;
        float centerY = (cellY * CELL_SIZE) + (CELL_SIZE / 2) + mazeOffset.y;

        // Calculate distance from cell center
        float distFromCenterX = std::abs(position.x - centerX);
        float distFromCenterY = std::abs(position.y - centerY);

        // If we're close to the center of a cell and haven't paused here yet, check if it's a pause position
        if (!hasPausedOnCurrentTile && distFromCenterX < 5.0f && distFromCenterY < 5.0f) {
            // Check if the current cell is in our list of pause positions
            for (const auto& pos : pausePositions) {
                if (pos.x == cellX && pos.y == cellY) {
                    // Pause the ghost
                    isPaused = true;
                    pauseTimer = 0.0f;
                    // Center the ghost precisely on the 'o' tile
                    position.x = centerX;
                    position.y = centerY;
                    sprite.setPosition(position);

                    // Mark that we've paused on this tile to prevent repeated pausing
                    lastPauseTile = sf::Vector2i(cellX, cellY);
                    hasPausedOnCurrentTile = true;

                    // Print debug info
                    std::cout << "Ghost paused at position: ("
                        << position.x << ", " << position.y
                        << "), cell: (" << cellX << ", " << cellY << ")" << std::endl;
                    return;
                }
            }
        }

        // Regular movement logic from Ghost class
        Ghost::updateAutonomous(maze);
    }

    bool isPauseActive() const {
        return isPaused;
    }

    void Reset() override {
        Ghost::Reset();
        isPaused = false;
        pauseTimer = 0.0f;
        lastPauseTile = sf::Vector2i(-1, -1);
        hasPausedOnCurrentTile = false;
    }
};


class TimeStopGhost : public Ghost {
private:
    // Constants for time stop ability
    const float TIME_STOP_COOLDOWN = 30.0f;    // Seconds between ability uses
    const float TIME_STOP_DURATION = 3.0f;     // How long Pacman is stopped
    const float WARNING_DURATION = 2.0f;       // Duration of warning flicker before ability activates

    // Timers
    float abilityTimer;                        // Tracks cooldown for time stop ability
    float stopDurationTimer;                   // Tracks how long Pacman has been stopped

    // State flags
    bool isWarning;                            // True when the ghost is about to use its ability
    bool isTimeStopActive;                     // True when time stop is currently active
    float warningBlinkTimer;                   // For flicker effect during warning
    const float BLINK_SPEED = 0.1f;            // How fast the ghost blinks during warning (seconds)

    // Original color for restoration
    sf::Color abilityColor;                    // Color when ability is active

    // Reference to pacman (optional, can be passed to update method instead)
    Pacman* targetPacman;

public:
    TimeStopGhost(const std::string& spriteSheetPath,
        int frameCount, int frameWidth, int frameHeight,
        float x, float y, float speed, float scale,
        const std::map<Direction, int>& frameIndexes)
        : Ghost(spriteSheetPath, frameCount, frameWidth, frameHeight, x, y, speed, scale, frameIndexes),
        abilityTimer(0.0f),
        stopDurationTimer(0.0f),
        isWarning(false),
        isTimeStopActive(false),
        warningBlinkTimer(0.0f),
        targetPacman(nullptr)
    {
        // Initialize ability color (light blue for time theme)
        abilityColor = sf::Color(100, 200, 255);
    }

    // Set the target Pacman
    void SetTarget(Pacman* pacman) {
        targetPacman = pacman;
    }

    void Update(float deltaTime) override {
        // Call the parent update method first
        Ghost::Update(deltaTime);

        // Update ability timer
        abilityTimer += deltaTime;

        // Check if warning phase should start
        if (!isWarning && !isTimeStopActive && abilityTimer >= TIME_STOP_COOLDOWN - WARNING_DURATION) {
            isWarning = true;
            warningBlinkTimer = 0.0f;
        }

        // Handle warning flicker effect
        if (isWarning) {
            warningBlinkTimer += deltaTime;

            // Flicker between original color and ability color (faster as time approaches)
            float blinkFrequency = BLINK_SPEED * (1.0f - ((TIME_STOP_COOLDOWN - abilityTimer) / WARNING_DURATION));
            blinkFrequency = std::max(blinkFrequency, BLINK_SPEED); // Don't go slower than minimum speed

            if (static_cast<int>(warningBlinkTimer / blinkFrequency) % 2 == 0) {
                setColor(originalColor);
            }
            else {
                setColor(abilityColor);
            }

            // Check if it's time to activate ability
            if (abilityTimer >= TIME_STOP_COOLDOWN) {
                ActivateTimeStop();
            }
        }

        // Handle active time stop
        if (isTimeStopActive) {
            stopDurationTimer += deltaTime;

            // Make sure Pacman remains stopped if we have a reference to him
            if (targetPacman != nullptr) {
                // Use Pacman's Stop method with his current direction
                targetPacman->Stop(targetPacman->GetDirection());
            }

            // End the time stop when duration is over
            if (stopDurationTimer >= TIME_STOP_DURATION) {
                DeactivateTimeStop();
            }
        }
    }

    // Alternative Update method that takes a Pacman reference directly
    void Update(float deltaTime, Pacman& pacman) {
        // Store temporary reference for this frame
        targetPacman = &pacman;
        Update(deltaTime);
    }

    // Method to activate the time stop ability
    void ActivateTimeStop() {
        isWarning = false;
        isTimeStopActive = true;
        stopDurationTimer = 0.0f;
        setColor(abilityColor);

        // Visual effect - ghost glows with time energy
        sprite.setColor(sf::Color(abilityColor.r, abilityColor.g, abilityColor.b, 255));
    }

    // Method to deactivate the time stop
    void DeactivateTimeStop() {
        isTimeStopActive = false;
        abilityTimer = 0.0f; // Reset the cooldown
        setColor(originalColor);
    }

    // Check if time stop is currently active
    bool IsTimeStopActive() const {
        return isTimeStopActive;
    }

    // Override collision to handle Pacman stopping on contact
    bool GhostCollision(const sf::Vector2f& pacmanPosition) const override {
        bool collision = Ghost::GhostCollision(pacmanPosition);

        // If we're colliding and the time stop isn't already active,
        // we could potentially trigger the ability immediately
        // (leaving this commented out as it depends on your game design)
        /*
        if (collision && !isTimeStopActive && !isWarning && abilityTimer > TIME_STOP_COOLDOWN * 0.5f) {
            // Could force ability to activate early on collision
            // const_cast<TimeStopGhost*>(this)->ActivateTimeStop();
        }
        */

        return collision;
    }

    void Reset() override {
        Ghost::Reset();
        abilityTimer = 0.0f;
        stopDurationTimer = 0.0f;
        isWarning = false;
        isTimeStopActive = false;
        warningBlinkTimer = 0.0f;
        // Don't need to reset targetPacman as it's a reference
    }

    // Float representing progress toward ability activation (0.0 to 1.0)
    float GetAbilityProgress() const {
        return abilityTimer / TIME_STOP_COOLDOWN;
    }

    // Forces the ability to activate immediately (for testing or special events)
    void ForceActivate() {
        abilityTimer = TIME_STOP_COOLDOWN;
        isWarning = false;
        ActivateTimeStop();
    }
};

class ChaserGhost : public Ghost {
private:
    float rageTriggerTimer;
    float rageDurationTimer;
    bool isRaging;

    const float RAGE_TRIGGER_TIME = 20.0f;
    const float RAGE_DURATION = 2.0f;

public:
    ChaserGhost(const std::string& spriteSheetPath,
        int frameCount, int frameWidth, int frameHeight,
        float x, float y, float speed, float scale,
        const std::map<Direction, int>& frameIndexes)
        : Ghost(spriteSheetPath, frameCount, frameWidth, frameHeight, x, y, speed, scale, frameIndexes),
        rageTriggerTimer(0.0f),
        rageDurationTimer(0.0f),
        isRaging(false)
    {
        // Ensure the original speed and color are stored
        setSpeed(speed); // Initializes current speed
    }

    void Update(float deltaTime) override {
        Ghost::Update(deltaTime);
        rageTriggerTimer += deltaTime;

        if (!isRaging && rageTriggerTimer >= RAGE_TRIGGER_TIME) {
            isRaging = true;
            rageDurationTimer = 0.0f;
            setSpeed(getOriginalSpeed() * 3.0f);
            setColor(sf::Color(255, 60, 60)); // Rage tint
        }

        if (isRaging) {
            rageDurationTimer += deltaTime;
            if (rageDurationTimer >= RAGE_DURATION) {
                isRaging = false;
                rageTriggerTimer = 0.0f;
                setSpeed(speed/3.0f);             // Reset speed
                setColor(getOriginalColor());             // Reset color
            }
        }
    }

    void Reset() override {
        Ghost::Reset();
        rageTriggerTimer = 0.0f;
        rageDurationTimer = 0.0f;
        isRaging = false;
        setSpeed(speed/3.0f);
        setColor(getOriginalColor());
    }

    bool getIsRaging() const { return isRaging; }
};