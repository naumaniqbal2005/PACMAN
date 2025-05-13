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
    const float AMBUSH_WAIT_TIME = 3.0f;  // Seconds to wait at ambush points
    float ambushTimer;                    // Timer for tracking wait time
    bool isWaiting;                       // Flag to indicate if ghost is waiting at ambush point

    // Store all ambush points (coordinates of 'o' in the map)
    std::vector<sf::Vector2f> ambushPoints;
    int currentTargetIndex;               // Index of the current ambush point target

public:
    AmbusherGhost(const std::string& spriteSheetPath,
        int frameCount, int frameWidth, int frameHeight,
        float x, float y, float speed, float scale,
        const std::map<Direction, int>& frameIndexes)
        : Ghost(spriteSheetPath, frameCount, frameWidth, frameHeight, x, y, speed, scale, frameIndexes),
        ambushTimer(0.0f),
        isWaiting(false),
        currentTargetIndex(0) {

        // Manually set the coordinates of the ambush points ('o' in the map)
        // Based on the provided map, these are the 'o' coordinates (converting to pixel coordinates)
        // The format is: x = column * CELL_SIZE + CELL_SIZE/2, y = row * CELL_SIZE + CELL_SIZE/2

        // First 'o' at (2, 2)
        ambushPoints.push_back(sf::Vector2f(2 * CELL_SIZE + CELL_SIZE / 2, 2 * CELL_SIZE + CELL_SIZE / 2));

        // Second 'o' at (17, 2)
        ambushPoints.push_back(sf::Vector2f(17 * CELL_SIZE + CELL_SIZE / 2, 2 * CELL_SIZE + CELL_SIZE / 2));

        // Third 'o' at (2, 15)
        ambushPoints.push_back(sf::Vector2f(2 * CELL_SIZE + CELL_SIZE / 2, 15 * CELL_SIZE + CELL_SIZE / 2));

        // Fourth 'o' at (17, 15)
        ambushPoints.push_back(sf::Vector2f(17 * CELL_SIZE + CELL_SIZE / 2, 15 * CELL_SIZE + CELL_SIZE / 2));
    }

    void updateAutonomous(Maze& maze) override {
        float deltaTime = 1.0f / 60.0f;

        // Check if currently waiting at an ambush point
        if (isWaiting) {
            ambushTimer += deltaTime;

            // Continue waiting until timer is up
            if (ambushTimer < AMBUSH_WAIT_TIME) {
                // Update animation while waiting
                Update(deltaTime);
                return;
            }
            else {
                // Done waiting, resume movement and target the next ambush point
                isWaiting = false;
                currentTargetIndex = (currentTargetIndex + 1) % ambushPoints.size();
            }
        }

        // Get current position in grid coordinates
        int currentCellX = static_cast<int>(position.x / CELL_SIZE);
        int currentCellY = static_cast<int>(position.y / CELL_SIZE);

        // Get current target ambush point
        sf::Vector2f targetPoint = ambushPoints[currentTargetIndex];
        int targetCellX = static_cast<int>(targetPoint.x / CELL_SIZE);
        int targetCellY = static_cast<int>(targetPoint.y / CELL_SIZE);

        // Check if we've reached the current target ambush point
        if (currentCellX == targetCellX && currentCellY == targetCellY && isAtCellCenter()) {
            // Start waiting at this ambush point
            isWaiting = true;
            ambushTimer = 0.0f;
            Update(deltaTime);
            return;
        }

        // If not at target, move toward it using A* or simpler pathfinding
        Direction bestDirection = calculateBestDirection(maze, targetPoint);

        // Try to move in best direction
        if (Move(bestDirection, maze)) {
            // Successfully moved in best direction
        }
        else {
            // If blocked, try alternate directions
            std::vector<Direction> possibleDirs;

            for (int d = 0; d < 4; ++d) {
                Direction dir = static_cast<Direction>(d);
                if (dir == getOpposite(currentDirection)) continue;

                if (isValidDirection(maze, dir)) {
                    possibleDirs.push_back(dir);
                }
            }

            if (!possibleDirs.empty()) {
                // Choose a random valid direction if best direction is blocked
                currentDirection = possibleDirs[std::rand() % possibleDirs.size()];
                Move(currentDirection, maze);
            }
        }

        Update(deltaTime);
    }

    // Calculate the best direction to move toward the target point
    Direction calculateBestDirection(Maze& maze, sf::Vector2f targetPoint) {
        // Simplistic approach: choose direction that gets us closest to target
        std::vector<Direction> possibleDirections;
        std::vector<float> distances;

        // Check each possible direction
        for (int d = 0; d < 4; ++d) {
            Direction dir = static_cast<Direction>(d);

            // Skip the opposite direction (no backtracking)
            if (dir == getOpposite(currentDirection) && !isAtIntersection(maze)) {
                continue;
            }

            // Check if this direction is valid
            if (isValidDirection(maze, dir)) {
                // Calculate position after moving in this direction
                sf::Vector2f newPos = position;
                switch (dir) {
                case RIGHT: newPos.x += CELL_SIZE; break;
                case LEFT:  newPos.x -= CELL_SIZE; break;
                case UP:    newPos.y -= CELL_SIZE; break;
                case DOWN:  newPos.y += CELL_SIZE; break;
                }

                // Calculate distance to target from new position
                float dx = newPos.x - targetPoint.x;
                float dy = newPos.y - targetPoint.y;
                float distance = std::sqrt(dx * dx + dy * dy);

                possibleDirections.push_back(dir);
                distances.push_back(distance);
            }
        }

        // Find direction with shortest distance to target
        if (!possibleDirections.empty()) {
            int bestIndex = 0;
            float minDistance = distances[0];

            for (size_t i = 1; i < distances.size(); ++i) {
                if (distances[i] < minDistance) {
                    minDistance = distances[i];
                    bestIndex = i;
                }
            }

            return possibleDirections[bestIndex];
        }

        // If no valid directions, keep current direction
        return currentDirection;
    }

    // Check if ghost is at an intersection (more than one possible direction)
    bool isAtIntersection(Maze& maze) const {
        int validDirections = 0;

        for (int d = 0; d < 4; ++d) {
            Direction dir = static_cast<Direction>(d);
            Vector2f testPos = position;

            // Move one cell in the given direction
            switch (dir) {
            case RIGHT: testPos.x += CELL_SIZE; break;
            case LEFT:  testPos.x -= CELL_SIZE; break;
            case UP:    testPos.y -= CELL_SIZE; break;
            case DOWN:  testPos.y += CELL_SIZE; break;
            }

            if (maze.isWalkable(testPos)) {
                validDirections++;
            }
        }

        // An intersection has more than 2 valid directions
        return validDirections > 2;
    }

    // Helper method to check if ghost is centered enough in a cell
    bool isAtCellCenter() const {
        float cellCenterX = std::floor(position.x / CELL_SIZE) * CELL_SIZE + (CELL_SIZE / 2);
        float cellCenterY = std::floor(position.y / CELL_SIZE) * CELL_SIZE + (CELL_SIZE / 2);

        // Define threshold for being "centered enough" (a small value, e.g., 5 pixels)
        const float CENTERED_THRESHOLD = 5.0f;

        return (std::abs(position.x - cellCenterX) < CENTERED_THRESHOLD &&
            std::abs(position.y - cellCenterY) < CENTERED_THRESHOLD);
    }

    void Reset() override {
        Ghost::Reset();
        ambushTimer = 0.0f;
        isWaiting = false;
        currentTargetIndex = 0; // Reset to target the first ambush point
    }
};