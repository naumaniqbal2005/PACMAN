#include <SFML/Graphics.hpp>
#include "maze.h"
#include "pacman.h"
#include "Ghosts.h"
//#include "SubGhosts.h"
#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <map>
#include <string>
#include <algorithm>
#include <random>
#include <iostream>
#include <SFML/Audio.hpp>
#include <SFML/System.hpp>
#include <thread>
#include <fstream>
using namespace std;
using namespace sf;

const int windowWidth = 960;
const int windowHeight = 1050;
bool hasTimeStopGhost = false;
bool pacmanFrozen = false;
float nextFreezeTime = 5.0f;
float freezeDuration = 1.5f;
float freezeStart = 0.0f;
float gameTimer;

// Define the GhostInfo structure for storing ghost details
struct GhostInfo {
    string name;
    string description;
};

// Map to store information about each ghost type
map<string, GhostInfo> ghostInfoMap = {
    {"TELEPORTER", {"TELEPORTER", "TELEPORTS - One Second here,Next Second there"}},
    {"RANDOMGHOST", {"RANDOM", "RANDOM - What's even the point ?"}},
    {"CHASER", {"CHASER", "CHASES PACMAN - You may run but you will not hide"}},
    {"AMBUSHER", {"AMBUSHER", "CAMPS SUPERFOOD - No Even Further Beyond"}},
    {"HERMES", {"HERMES", "SWIFT - You may hide but you will not run"}},
    {"PHANTOM", {"PHANTOM", "EXTENDED HIT RADIUS - Loves his loneliness"}},
    {"TIMESTOP", {"TIME STOP", "PAUSES PACMAN - You shall not Pass"}},
    {"RINGGHOST", {"RING", "INVISIBILITY - You can't see him (sometimes)"}}
};

vector<string> ghostNames = {
    "TELEPORTER", "RANDOMGHOST", "CHASER", "AMBUSHER",
    "HERMES", "PHANTOM", "TIMESTOP", "RINGGHOST"
};

struct Dot {
    CircleShape shape;
    float speed;
    float angle;  // For circular motion
    float radius; // For circular motion
    Vector2f center; // Center point for circular motion
    float oscillation; // For pulsing
    bool isPulsing; // Whether this dot pulses
};

// Global music object (declared outside to avoid scope issues)
sf::Music menuMusic;
sf::Music superMusic;
void playMenuMusic() {
    // Load the music file
    if (!menuMusic.openFromFile("pm.ogg")) {
        std::cerr << "Error: Could not load pm.ogg\n";
        return;
    }

    // Set up the music properties
    menuMusic.setLoop(true);   // Ensure it loops
    menuMusic.setVolume(50);   // Adjust volume (0-100)

    // Play the music
    menuMusic.play();
}

void playSuperMusic() {
    // Load the music file
    if (!superMusic.openFromFile("ssj3.wav")) {
        std::cerr << "Error: Could not load ssj3.wav\n";
        return;
    }

    // Set up the music properties
    menuMusic.setVolume(75);   // Adjust volume (0-100)

    // Play the music
    superMusic.play();
}
void stopMenuMusic() {
    if (menuMusic.getStatus() == sf::Music::Playing) {
        menuMusic.stop();
    }
}
void stopsuperMusic() {
    if (superMusic.getStatus() == sf::Music::Playing) {
        superMusic.stop();
    }
}

void generateBackgroundDots(vector<Dot>& dots) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> radiusDist(100.0f, 350.0f);
    std::uniform_real_distribution<float> speedDist(0.5f, 2.5f);
    std::uniform_real_distribution<float> oscDist(0.0f, 2.0f * 3.14159f);
    std::bernoulli_distribution isPulsingDist(0.3f); // 30% chance for pulsing dots

    // Create standard moving dots
    for (int i = 0; i < 50; ++i) {
        CircleShape dot(2 + static_cast<float>(rand() % 3)); // Varied sizes

        // Random vibrant colors for some dots
        if (rand() % 5 == 0) { // 20% chance for colored dots
            dot.setFillColor(Color(rand() % 255, rand() % 255, rand() % 255, 150 + rand() % 100));
        }
        else {
            dot.setFillColor(Color(200, 200, 200, 150 + rand() % 100)); // White/gray with alpha
        }

        dot.setPosition(rand() % windowWidth, rand() % windowHeight);
        float speed = speedDist(gen);
        dots.push_back({ dot, speed, 0, 0, {0, 0}, 0, false });
    }

    // Create orbital dots
    for (int i = 0; i < 30; ++i) {
        CircleShape dot(1 + static_cast<float>(rand() % 2));
        dot.setFillColor(Color::Yellow);

        Vector2f center(windowWidth / 2.0f, 300.0f);
        float radius = radiusDist(gen);
        float angle = angleDist(gen);
        float speed = speedDist(gen) * 0.5f;

        // Calculate position based on center, radius and angle
        float x = center.x + radius * cos(angle);
        float y = center.y + radius * sin(angle);
        dot.setPosition(x, y);

        bool isPulsing = isPulsingDist(gen);
        float oscillation = oscDist(gen);

        dots.push_back({ dot, speed, angle, radius, center, oscillation, isPulsing });
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
    // Create ghosts with different starting positions and speeds
    for (float i = 0; i < 4; ++i) {
        string ghostName = ghostNames[i];
        string spriteSheetPath = ghostName + ".png";
        map<Direction, int> frameIndexes = {
            {RIGHT, 0}, {UP, 1}, {DOWN, 2}, {LEFT, 3}
        };

        // Stagger initial positions for a "chase" effect
        float x = -120.0f - (i * 150.0f);
        float y = 640.0f + fmod(i * 30.0f, 80.0f);  // Vary vertical positions slightly
        // Vary speeds slightly for more dynamic movement
        float ghostSpeed = 2.0f + (i * 0.5f);
        Ghost* g = new Ghost(spriteSheetPath, 4, 50, 50, x, y, ghostSpeed, 5.0f, frameIndexes);
        ghosts.push_back(g);
    }
    return ghosts;
}

void updateDots(vector<Dot>& dots, float dt) {
    for (auto& d : dots) {
        if (d.radius > 0) {
            // This is an orbital dot
            d.angle += d.speed * dt;

            // Calculate new position based on center, radius and updated angle
            float x = d.center.x + d.radius * cos(d.angle);
            float y = d.center.y + d.radius * sin(d.angle);

            d.shape.setPosition(x, y);

            // Handle pulsing effect
            if (d.isPulsing) {
                d.oscillation += dt * 3.0f;
                float scale = 0.7f + 0.3f * sin(d.oscillation);
                float currentRadius = d.shape.getRadius();
                d.shape.setRadius(currentRadius * scale);

                // Adjust color based on oscillation
                float brightness = 150 + 105 * sin(d.oscillation);
                d.shape.setFillColor(Color(255, 255, brightness, 200));
            }
        }
        else {
            // This is a standard moving dot
            Vector2f pos = d.shape.getPosition();
            pos.y += d.speed;
            if (pos.y > windowHeight) pos.y = 0;
            d.shape.setPosition(pos);
        }
    }
}

void spawnGameGhosts(vector<Ghost*>& gameGhosts, const Maze& maze, vector<string>& selectedGhosts) {
    vector<string> ghostNames = {
        "RANDOMGHOST", "CHASER", "AMBUSHER", "PHANTOM",
        "HERMES", "RINGGHOST","TELEPORTER", "TIMESTOP"
    };

    random_device rd;
    mt19937 gen(rd());
    shuffle(ghostNames.begin(), ghostNames.end(), gen);

    // Clear the selected ghosts list and add the first 4 ghost types
    selectedGhosts.clear();
    for (int i = 0; i < 4; ++i) {
        selectedGhosts.push_back(ghostNames[i]);
    }

    for (int i = 0; i < 4; ++i) {
        string ghostName = ghostNames[i];
        string spriteSheetPath =ghostName + ".png";

        map<Direction, int> frameIndexes = {
            {RIGHT, 0}, {UP, 1}, {DOWN, 2}, {LEFT, 3}
        };

        Vector2i ghostPos = maze.getGhost(i + '0');

        if (ghostPos.x == -1 || ghostPos.y == -1) continue;

        static const int TILE_SIZE = 40;
        float x = ghostPos.x * TILE_SIZE;
        float y = ghostPos.y * TILE_SIZE;

        if (ghostName == "HERMES")
        {
            Ghost* g = new Ghost(spriteSheetPath, 4, 50, 50,
                x + TILE_SIZE / 2, y + TILE_SIZE / 2, 3.5f, 1.3f,
                frameIndexes);
            gameGhosts.push_back(g);
        }
        else if (ghostName == "RINGGHOST") {
            Ghost* g = new RingGhost(spriteSheetPath, 4, 50, 50,
                x + TILE_SIZE / 2, y + TILE_SIZE / 2, 2.5f, 1.3f,
                frameIndexes);
            gameGhosts.push_back(g);
        }
        else if (ghostName == "TELEPORTER") {
            Ghost* g = new TeleporterGhost(spriteSheetPath, 4, 50, 50,
                x + TILE_SIZE / 2, y + TILE_SIZE / 2, 2.5f, 1.3f,
                frameIndexes);
            gameGhosts.push_back(g);
        }
        else if (ghostName == "PHANTOM") {
            Ghost* g = new Ghost(spriteSheetPath, 4, 50, 50,
                x + TILE_SIZE / 2, y + TILE_SIZE / 2, 2.5f, 1.3f,
                frameIndexes);
            gameGhosts.push_back(g);
        }
        else if (ghostName == "AMBUSHER") {
            Ghost* g = new AmbusherGhost(spriteSheetPath, 4, 50, 50,
                x + TILE_SIZE / 2, y + TILE_SIZE / 2, 2.5f, 1.3f,
                frameIndexes);
            gameGhosts.push_back(g);
        }
        else if (ghostName == "TIMESTOP") {
            Ghost* g = new TimeStopGhost(spriteSheetPath, 4, 50, 50,
                x + TILE_SIZE / 2, y + TILE_SIZE / 2, 2.5f, 1.3f,
                frameIndexes);
            gameGhosts.push_back(g);
            hasTimeStopGhost = true;
        }
        else if(ghostName == "CHASER")
        {
            Ghost* g = new ChaserGhost(spriteSheetPath, 4, 50, 50,
                x + TILE_SIZE / 2, y + TILE_SIZE / 2, 2.5f, 1.3f,
                frameIndexes);
            gameGhosts.push_back(g);

        }
        else
        {
            Ghost* g = new Ghost(spriteSheetPath, 4, 50, 50,
                x + TILE_SIZE / 2, y + TILE_SIZE / 2, 2.5f, 1.3f,
                frameIndexes);
            gameGhosts.push_back(g);

        }
    }
}

void displayGhostAbilities(RenderWindow& window, const Font& font, const vector<string>& selectedGhosts,
    vector<Dot>& backgroundDots, float dt) {
    Clock displayClock;
    float displayTime = 0.0f;
    const float DISPLAY_DURATION = 10.0f; // Show for 5 seconds

    // Create background with dots
    while (displayTime < DISPLAY_DURATION && window.isOpen()) {
        displayTime = displayClock.getElapsedTime().asSeconds();

        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed)
                window.close();
            if (event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::Return || event.key.code == Keyboard::Space) {
                    // Allow skipping with Enter or Space
                    return;
                }
            }
        }

        window.clear(Color(0, 0, 0)); // Dark blue background

        // Update background dots
        updateDots(backgroundDots, dt);
        for (auto& d : backgroundDots)
            window.draw(d.shape);

        // Draw title
        Text title("BEWARE OF THESE GHOSTS!", font, 48);
        title.setFillColor(Color::Yellow);
        title.setPosition(windowWidth / 2.f - title.getGlobalBounds().width / 2.f, 110);
        window.draw(title);

        // Draw ghost information
        for (int i = 0; i < selectedGhosts.size() && i < 4; ++i) {
            string ghostType = selectedGhosts[i];

            // Create ghost sprite preview
            Sprite ghostSprite;
            Texture ghostTexture;
            if (ghostTexture.loadFromFile(ghostType + ".png")) {
                ghostSprite.setTexture(ghostTexture);
                ghostSprite.setTextureRect(IntRect(0, 0, 50, 50)); // First frame
                ghostSprite.setScale(2.0f, 2.0f);
                ghostSprite.setPosition(30, 220 + i * 170);
                window.draw(ghostSprite);
            }

            // Ghost name
            Text nameText(ghostInfoMap[ghostType].name, font, 36);
            nameText.setFillColor(Color::White);
            nameText.setPosition(110, 210 + i * 160);
            window.draw(nameText);

            // Ghost description
            Text descText(ghostInfoMap[ghostType].description, font, 24);
            descText.setFillColor(Color(200, 200, 200));
            descText.setPosition(150, 250 + i * 170);
            window.draw(descText);
        }

        // Skip instruction
        Text skipText("PRESS ENTER OR SPACE TO CONTINUE", font, 24);
        skipText.setFillColor(Color(150, 150, 150));
        skipText.setPosition(windowWidth / 2.f - skipText.getGlobalBounds().width / 2.f,
            windowHeight - 100);

        // Make the text blink
        if (static_cast<int>(displayTime * 2) % 2 == 0) {
            window.draw(skipText);
        }

        window.display();
    }
}

void displayGhostInstructions(RenderWindow& window, const Font& font,
    vector<Dot>& backgroundDots, float dt, bool& instructions, bool& isMenu) {


    // Load ghost textures
    map<string, Texture> ghostTextures;
    for (const auto& ghostName : ghostNames) {
        Texture texture;
        if (texture.loadFromFile(ghostName + ".png")) {
            ghostTextures[ghostName] = texture;
        }
    }

    // Create background with dots
    while (instructions) {


        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed)
                window.close();
            if (event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::Enter || event.key.code == Keyboard::Return) {
                    instructions = false;
                    isMenu = true;
                    return;
                }
            }
        }

        window.clear(Color(0, 0, 0)); // Black background

        // Update background dots
        updateDots(backgroundDots, dt);
        for (auto& d : backgroundDots)
            window.draw(d.shape);

        // Draw title
        Text title("KNOW YOUR ENEMIES", font, 36);  // Smaller title
        title.setFillColor(Color::Red);
        title.setPosition(windowWidth / 2.f - title.getGlobalBounds().width / 2.f, 20);  // Positioned higher
        window.draw(title);

        // Draw subtitle
        Text subtitle("GHOST ABILITIES", font, 28);  // Smaller subtitle
        subtitle.setFillColor(Color::White);
        subtitle.setPosition(windowWidth / 2.f - subtitle.getGlobalBounds().width / 2.f, 60);  // Positioned higher
        window.draw(subtitle);

        // Draw ghost information in a vertical list (single column layout)
        const int ROW_HEIGHT = 70;  // Reduced row height
        const int START_Y = 100;    // Start higher on the screen

        for (int i = 0; i < ghostNames.size(); ++i) {
            string ghostType = ghostNames[i];
            float x = 30;  // Left margin
            float y = START_Y + i * ROW_HEIGHT;

            // Create ghost sprite
            if (ghostTextures.find(ghostType) != ghostTextures.end()) {
                Sprite ghostSprite;
                ghostSprite.setTexture(ghostTextures[ghostType]);
                ghostSprite.setTextureRect(IntRect(0, 0, 50, 50)); // First frame
                ghostSprite.setScale(1.0f, 1.0f);  // Smaller scale
                ghostSprite.setPosition(x, y);
                window.draw(ghostSprite);
            }

            // Ghost name
            Text nameText(ghostInfoMap[ghostType].name, font, 22);  // Smaller text
            nameText.setFillColor(Color::Cyan);
            nameText.setPosition(x + 60, y);  // Closer to sprite
            window.draw(nameText);

            // Ghost description
            Text descText(ghostInfoMap[ghostType].description, font, 16);  // Smaller text
            descText.setFillColor(Color(200, 200, 200));
            descText.setPosition(x + 60, y + 25);  // Closer to name
            window.draw(descText);
        }

        // Draw super mode text at bottom
        Text superModeTitle("SUPER MODE", font, 28);  // Smaller title
        superModeTitle.setFillColor(Color::Yellow);
        superModeTitle.setPosition(windowWidth / 2.f - superModeTitle.getGlobalBounds().width / 2.f, windowHeight - 150);
        window.draw(superModeTitle);

        Text superModeText("EAT YELLOW SUPER FOOD TO GO EVEN FURTHER BEYOND", font, 18);  // Smaller text
        superModeText.setFillColor(Color::Yellow);
        superModeText.setPosition(windowWidth / 2.f - superModeText.getGlobalBounds().width / 2.f, windowHeight - 120);
        window.draw(superModeText);

        Text superModeText2("AND EAT GHOSTS AND INCREASE SPEED", font, 18);  // Smaller text
        superModeText2.setFillColor(Color::Yellow);
        superModeText2.setPosition(windowWidth / 2.f - superModeText2.getGlobalBounds().width / 2.f, windowHeight - 100);
        window.draw(superModeText2);

        // Skip instruction
        Text skipText("PRESS ENTER TO CONTINUE", font, 20);  // Smaller text
        skipText.setFillColor(Color(150, 150, 150));
        skipText.setPosition(windowWidth / 2.f - skipText.getGlobalBounds().width / 2.f, windowHeight - 70);



        window.draw(skipText);


        window.display();
    }
}
void drawCountdown(RenderWindow& window, Font& font, int countdownStage) {
    Text countdownText;
    countdownText.setFont(font);
    countdownText.setCharacterSize(72);
    countdownText.setFillColor(Color::Yellow);

    // Select the text based on countdown stage
    switch (countdownStage) {
    case 0:
        countdownText.setString("Ready");
        break;
    case 1:
        countdownText.setString("Set");
        break;
    case 2:
        countdownText.setString("Go!");
        break;
    default:
        return; // Don't draw anything if countdown is done
    }

    // Center the text
    FloatRect textBounds = countdownText.getLocalBounds();
    countdownText.setOrigin(textBounds.width / 2, textBounds.height / 2);
    countdownText.setPosition(window.getSize().x / 2, window.getSize().y / 2-15);

    // Draw the text
    window.draw(countdownText);
}

void drawMenu(RenderWindow& window, Text& title, vector<Text>& menuTexts, int selectedItem,
    vector<Ghost*>& menuGhosts, vector<Dot>& dots, float dt, bool inMenu) {

    // Draw background
    for (auto& d : dots)
        window.draw(d.shape);

    // Create a pulsing effect for the title
    static float titlePulseTimer = 0.0f;
    titlePulseTimer += dt;
    float titleScale = 1.0f + 0.05f * sin(titlePulseTimer * 3.0f);
    title.setScale(titleScale, titleScale);

    // Smoothly change title color
    int r = 255;  // Keep red at max for yellow
    int g = 255;  // Keep green at max for yellow
    int b = static_cast<int>(60 + 40 * sin(titlePulseTimer * 2.0f));  // Subtle blue pulsing
    title.setFillColor(Color(r, g, b));

    // Draw title with drop shadow for better visibility
    Text shadowTitle = title;
    shadowTitle.setFillColor(Color(30, 30, 30, 150));
    shadowTitle.setPosition(title.getPosition() + Vector2f(3, 3));
    window.draw(shadowTitle);
    window.draw(title);

    // Draw menu options with hover effect
    for (size_t i = 0; i < menuTexts.size(); ++i) {
        // Set the base color
        Color baseColor = (i == selectedItem) ? Color::Yellow : Color::White;

        // Add pulsing effect to selected item
        if (i == selectedItem) {
            float pulseValue = 0.7f + 0.3f * sin(titlePulseTimer * 5.0f);
            baseColor = Color(
                static_cast<Uint8>(255 * pulseValue),
                static_cast<Uint8>(255 * pulseValue),
                static_cast<Uint8>(50)
            );

            // Make selected item larger
            menuTexts[i].setScale(1.1f, 1.1f);

            // Add an arrow cursor - FIX: Use two separate characters ">>" instead of a string literal
            Text arrow;
            arrow.setString(">");  // Single character
            arrow.setFont(*menuTexts[i].getFont());
            arrow.setCharacterSize(40);
            arrow.setFillColor(baseColor);

            // Position the first arrow
            arrow.setPosition(
                menuTexts[i].getPosition().x - arrow.getGlobalBounds().width - 15,
                menuTexts[i].getPosition().y
            );
            window.draw(arrow);

            // Position the second arrow
            arrow.setPosition(
                menuTexts[i].getPosition().x - arrow.getGlobalBounds().width * 2 - 10,
                menuTexts[i].getPosition().y
            );
            window.draw(arrow);
        }
        else {
            menuTexts[i].setScale(1.0f, 1.0f);
        }

        menuTexts[i].setFillColor(baseColor);

        // Draw shadow for better visibility
        Text shadowText = menuTexts[i];
        shadowText.setFillColor(Color(30, 30, 30, 150));
        shadowText.setPosition(menuTexts[i].getPosition() + Vector2f(2, 2));
        shadowText.setScale(menuTexts[i].getScale());
        window.draw(shadowText);

        window.draw(menuTexts[i]);
    }

    if (inMenu) {
        // Move and draw the menu ghosts
        for (int i = 0; i < static_cast<int>(menuGhosts.size()); ++i) {
            Ghost* g = menuGhosts[i];

            // Move the ghost in the RIGHT direction
            g->menMove(RIGHT);

            // Update ghost's movement
            g->Update(dt);

            if (g->GetPosition().x > windowWidth) {
                // Calculate new x-coordinate based on ghost index
                float newX = -190.f - (rand() % 100);  // Add some randomness
                float newY = 640.f + (rand() % 80);    // Vary vertical position too
                g->SetPosition(newX, newY);
            }

            window.draw(g->getSprite());
        }

        // Draw "Press Enter to Start" flashing text - FIX: Use a more appropriate format
        static float flashTimer = 0.0f;
        flashTimer += dt;
        if (sin(flashTimer * 3.0f) > 0) {  // Flash at 3Hz
            Text pressEnter;
            pressEnter.setString("PRESS ENTER TO START");
            pressEnter.setFont(*menuTexts[0].getFont());
            pressEnter.setCharacterSize(30);
            pressEnter.setFillColor(Color::White);

            // Center the text
            pressEnter.setPosition(
                windowWidth / 2.f - pressEnter.getGlobalBounds().width / 2.f,
                650
            );

            // Add a shadow for better visibility
            Text shadowPressEnter = pressEnter;
            shadowPressEnter.setFillColor(Color(30, 30, 30, 150));
            shadowPressEnter.setPosition(pressEnter.getPosition() + Vector2f(2, 2));
            window.draw(shadowPressEnter);
            window.draw(pressEnter);
        }
    }
}

void drawUI(RenderWindow& window, const Font& font, int score,int highscore, int lives, bool superMode, float superModeTimer) {
    // Draw score at the top

    Text highscoreText("HIGHSCORE: " + to_string(highscore), font, 30);
    highscoreText.setFillColor(Color::Yellow);
    highscoreText.setPosition(325, 40);

    // Drop shadow for better visibility
    Text highScore = highscoreText;
    highScore.setFillColor(Color(30, 30, 30, 150));
    highScore.setPosition(highscoreText.getPosition() + Vector2f(2, 2));
    window.draw(highScore);
    window.draw(highscoreText);

    Text scoreText("SCORE: " + to_string(score), font, 30);
    scoreText.setFillColor(Color::White);
    scoreText.setPosition(50, 900);

    // Drop shadow for better visibility
    Text shadowScore = scoreText;
    shadowScore.setFillColor(Color(30, 30, 30, 150));
    shadowScore.setPosition(scoreText.getPosition() + Vector2f(2, 2));
    window.draw(shadowScore);
    window.draw(scoreText);

    // Draw lives
    Text livesText("LIVES: " + to_string(lives), font, 30);
    livesText.setFillColor(Color::White);
    livesText.setPosition(windowWidth - livesText.getGlobalBounds().width - 50, 900);

    // Drop shadow
    Text shadowLives = livesText;
    shadowLives.setFillColor(Color(30, 30, 30, 150));
    shadowLives.setPosition(livesText.getPosition() + Vector2f(2, 2));
    window.draw(shadowLives);
    window.draw(livesText);

    // If super mode is active, show timer
    if (superMode) {

        int timerSeconds = static_cast<int>(superModeTimer);
        Text superText("SUPER MODE: " + to_string(timerSeconds), font, 30);
        superText.setFillColor(Color::Yellow);
        superText.setPosition(
            windowWidth / 2.f - superText.getGlobalBounds().width / 2.f,
            930
        );

        // Pulsating effect for super mode
        static float pulseTimer = 0.0f;
        pulseTimer += 0.1f;
        float scale = 1.0f + 0.1f * sin(pulseTimer * 5.0f);
        superText.setScale(scale, scale);

        // Drop shadow
        Text shadowSuper = superText;
        shadowSuper.setFillColor(Color(30, 30, 30, 150));
        shadowSuper.setPosition(superText.getPosition() + Vector2f(2, 2));
        shadowSuper.setScale(superText.getScale());
        window.draw(shadowSuper);
        window.draw(superText);
    }
}
// Helper function to draw countdown when a life is lost
void drawLifeLostCountdown(RenderWindow& window, Font& font, float remainingTime) {
    Text countdownText(to_string((int)remainingTime + 1), font, 80);
    countdownText.setFillColor(Color::Yellow);
    countdownText.setPosition(window.getSize().x / 2, window.getSize().y / 2 - 15);
    window.draw(countdownText);
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

    vector<string> menuItems = { "Start Game", "Instructions", "Exit" };
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

    Pacman pacman(pacPaths, 4, 50, 50, pacmanStartPos.x, pacmanStartPos.y, 2.5f);

    vector<Ghost*> menuGhosts = createMenuGhosts();
    vector<Ghost*> gameGhosts;
    vector<string> selectedGhosts;

    bool inMenu = true;
    bool gameStarted = false;
    bool gameOver = false;
    bool instructions = false;
    int score = 0;
    int lives = 3;
    bool superMode = false;
    float superModeTimer = 0.0f;
    const float SUPER_MODE_DURATION = 10.0f; // 10 seconds of super mode

    // Ghost states for super mode
    vector<bool> ghostsBlinking(4, false);
    vector<float> ghostBlinkTimers(4, 0.0f);
    vector<Color> originalGhostColors(4, Color::White);
    vector<bool> ghostsReturnToSpawn(4, false);

    // Countdown variables
    bool countdownActive = false;
    float countdownTimer = 0.0f;
    int countdownStage = 0;  // 0 = "Ready", 1 = "Set", 2 = "Go", 3 = done
    const float COUNTDOWN_TIME_PER_STAGE = 1.0f;  // Each stage lasts 1 second

    // Life lost countdown variables
    bool lifeLostCountdown = false;
    float lifeLostTimer = 0.0f;
    const float LIFE_LOST_COUNTDOWN_DURATION = 3.0f;

    // Pacman death blinking variables
    bool pacmanDying = false;
    float pacmanDeathTimer = 0.0f;
    const float PACMAN_DEATH_DURATION = 3.0f;
    const float BLINK_RATE = 0.2f;  // How fast Pacman blinks (seconds)

    Clock clock;

    // Start menu music
    playMenuMusic();

    int highScore = 0;
    std::ifstream highScoreFileIn("highscore.txt");
    if (highScoreFileIn.is_open()) {
        highScoreFileIn >> highScore;
        highScoreFileIn.close();
    }

    while (window.isOpen()) {
        srand(static_cast<unsigned>(time(0)));
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

                            // Initialize countdown instead of starting game immediately
                            countdownActive = true;
                            countdownTimer = 0.0f;
                            countdownStage = 0;

                            // Stop menu music when game preparation starts
                            stopMenuMusic();

                            // Spawn ghosts and get the list of selected ghosts
                            spawnGameGhosts(gameGhosts, maze, selectedGhosts);
                            cout << "Game ghosts spawned: " << gameGhosts.size() << endl;

                            // Display ghost abilities screen
                            displayGhostAbilities(window, font, selectedGhosts, dots, dt);

                            // Store original ghost colors
                            for (size_t i = 0; i < gameGhosts.size() && i < originalGhostColors.size(); i++) {
                                originalGhostColors[i] = gameGhosts[i]->getSprite().getColor();
                            }

                            // Reset Pacman position
                            pacman.SetPosition(pacmanStartPos.x, pacmanStartPos.y);
                        }
                        else if (selectedItem == 1) {
                            // Show instructions
                            instructions = true;
                            inMenu = false;
                            // Display ghost instructions
                            displayGhostInstructions(window, font, dots, dt, instructions, inMenu);
                        }
                        else if (selectedItem == 2) {
                            window.close();
                        }
                    }
                }
                else if (gameStarted && !countdownActive && !lifeLostCountdown && !pacmanDying) {
                    if (event.key.code == Keyboard::Up)    pacman.SetDirection(UP);
                    if (event.key.code == Keyboard::Down)  pacman.SetDirection(DOWN);
                    if (event.key.code == Keyboard::Left)  pacman.SetDirection(LEFT);
                    if (event.key.code == Keyboard::Right) pacman.SetDirection(RIGHT);

                    // Add debug key for super mode testing
                    if (event.key.code == Keyboard::S) {
                        superMode = true;
                        superModeTimer = SUPER_MODE_DURATION;

                        // Change all ghosts to white
                        for (auto g : gameGhosts) {
                            g->setColor(Color::White);
                        }
                    }
                }
                else if (gameOver) {
                    if (event.key.code == Keyboard::Enter || event.key.code == Keyboard::Return) {
                        gameOver = false;
                        score = 0;
                        inMenu = true;
                    }
                }
            }
        }

        window.clear(Color::Black);

        // Update background dots
        updateDots(dots, dt);

        if (inMenu) {
            drawMenu(window, title, menuTexts, selectedItem, menuGhosts, dots, dt, inMenu);
        }
        else if (countdownActive) {
            // Draw the maze in the background during countdown
            maze.draw(window);

            // Update countdown timer
            countdownTimer += dt;

            if (countdownTimer >= COUNTDOWN_TIME_PER_STAGE) {
                countdownStage++;
                countdownTimer = 0.0f;

                // When countdown is finished
                if (countdownStage > 2) {
                    countdownActive = false;
                    gameStarted = true;  // Now actually start the game

                    // Start game music/sounds
                    //playGameMusic();
                }
            }

            // Draw countdown text
            drawCountdown(window, font, countdownStage);

            // Draw Pacman and ghosts in their initial positions
        }
        else if (lifeLostCountdown) {
            // Draw the maze in the background during life lost countdown
            // Draw Pacman and ghosts in their initial positions
            window.draw(pacman.getSprite());
            for (auto g : gameGhosts) {
                window.draw(g->getSprite());
            }

            maze.draw(window);

            // Update life lost countdown timer
            lifeLostTimer += dt;

            // Draw UI elements (score, lives, etc.)
            drawUI(window, font, score,highScore, lives, superMode, superModeTimer);

            // Draw Pacman and ghosts in their frozen positions
            window.draw(pacman.getSprite());
            for (auto g : gameGhosts) {
                window.draw(g->getSprite());
            }

            // Display "LIFE LOST" message
            Text lifeLostText("LIFE LOST", font, 40);
            lifeLostText.setFillColor(Color::Red);
            lifeLostText.setPosition(windowWidth / 2.f - lifeLostText.getGlobalBounds().width / 2.f, 340);
            window.draw(lifeLostText);

            // Display countdown text
            Text countdownText(to_string((int)(LIFE_LOST_COUNTDOWN_DURATION - lifeLostTimer) + 1), font, 80);
            countdownText.setFillColor(Color::Yellow);
            countdownText.setPosition(windowWidth / 2.f - countdownText.getGlobalBounds().width / 2.f, 400);
            window.draw(countdownText);

            // When countdown finishes
            if (lifeLostTimer >= LIFE_LOST_COUNTDOWN_DURATION) {
                lifeLostCountdown = false;
                lifeLostTimer = 0.0f;

                // If lives are gone, transition to pacman dying animation
                if (lives <= 0) {
                    pacmanDying = true;
                    pacmanDeathTimer = 0.0f;
                }
            }
        }
        else if (pacmanDying) {
            // Draw the maze in the background
            maze.draw(window);

            // Update pacman death timer
            pacmanDeathTimer += dt;

            // Make Pacman blink and become gradually transparent
            if ((int)(pacmanDeathTimer / BLINK_RATE) % 2 == 0) {
                // Calculate transparency level (fade out over time)
                int alpha = 255 * (1.0f - (pacmanDeathTimer / PACMAN_DEATH_DURATION));
                alpha = max(0, min(255, alpha)); // Clamp between 0-255

                pacman.setColor(Color(255, 255, 0, alpha)); // Yellow with decreasing alpha
            }
            else {
                pacman.setColor(Color(255, 255, 0, 0)); // Completely transparent
            }

            // Draw UI elements
            drawUI(window, font, score,highScore, 0, false, 0.0f);

            // Draw frozen ghosts
            for (auto g : gameGhosts) {
                window.draw(g->getSprite());
            }

            // Draw blinking Pacman
            window.draw(pacman.getSprite());

            // When death animation finishes
            if (pacmanDeathTimer >= PACMAN_DEATH_DURATION) {
                pacmanDying = false;
                gameOver = true;
                gameStarted = false;

                // Reset Pacman color
                pacman.setColor(Color(255, 255, 0, 255));

                // Reset game state
                lives = 3;
                superMode = false;

                // Clean up ghosts
                for (auto ghost : gameGhosts) {
                    delete ghost;
                }
                gameGhosts.clear();

                // Reset maze
                maze.reset();

                // Reset Pacman position
                pacman.SetPosition(pacmanStartPos.x, pacmanStartPos.y);

                // Restart menu music
                playMenuMusic();
            }
        }
        else if (gameStarted) {
            // Update super mode timer

            gameTimer += dt;

            if (superMode) {
                superModeTimer -= dt;
                if (superModeTimer <= 0) {
                    superMode = false;

                    // Reset ghost colors when super mode ends
                    for (size_t i = 0; i < gameGhosts.size() && i < originalGhostColors.size(); i++) {
                        if (!ghostsBlinking[i] && !ghostsReturnToSpawn[i]) {
                            gameGhosts[i]->setColor(originalGhostColors[i]);
                        }
                    }
                }
            }

            if (!superMode) {
                pacman.ResetScale();  // Reset Pacman scale when not in super mode 
                stopsuperMusic();  // Stop super mode music
            }

            if (hasTimeStopGhost && !pacmanFrozen && gameTimer >= nextFreezeTime) {
                pacmanFrozen = true;
                freezeStart = gameTimer;
                nextFreezeTime = gameTimer + 25.0f;  // Next freeze allowed after 25s
                pacman.Stop(pacman.GetDirection());
                cout << "[TimeStop] Pac-Man frozen at: " << gameTimer << endl;
            }

            if (pacmanFrozen && (gameTimer - freezeStart >= freezeDuration)) {
                pacmanFrozen = false;
                cout << "[TimeStop] Pac-Man unfrozen at: " << gameTimer << endl;
            }

            // --- Pac-Man logic ---
            if (pacmanFrozen == false) {
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

                // Food collection
                if (maze.isFood(pacman.GetPosition())) {
                    score += 10;
                }

                if (maze.isSuperFood(pacman.GetPosition())) {
                    score += 50;
                    superMode = true;
                    pacman.SuperScale();  // Scale up Pacman for super mode
                    superModeTimer = SUPER_MODE_DURATION;
                    playSuperMusic();

                    for (auto g : gameGhosts) {
                        g->setColor(Color::White);
                    }
                }

                pacman.Update();  // Only animate when active
            }

            // Draw maze - only when in game mode
            maze.draw(window);

            // Draw and update ghosts
            for (size_t i = 0; i < gameGhosts.size() && i < ghostsBlinking.size(); i++) {
                Ghost* g = gameGhosts[i];

                // Handle blinking ghosts
                if (ghostsBlinking[i]) {
                    ghostBlinkTimers[i] += dt;

                    // Blink effect - toggle visibility every 0.2 seconds
                    if (static_cast<int>(ghostBlinkTimers[i] * 5) % 2 == 0) {
                        g->setColor(Color::White);
                    }
                    else {
                        g->setColor(Color(255, 255, 255, 50));  // Semi-transparent instead of invisible
                    }

                    // After 2 seconds of blinking, return to spawn
                    if (ghostBlinkTimers[i] >= 2.0f) {
                        ghostsBlinking[i] = false;
                        ghostsReturnToSpawn[i] = true;
                        g->setColor(originalGhostColors[i]);  // Restore original color

                        // Set ghost to return to spawn point
                        Vector2i spawnPos = maze.getGhost('0');  // Use ghost 0's spawn position
                        g->SetPosition(
                            spawnPos.x * cellSize + cellSize / 2,
                            spawnPos.y * cellSize + cellSize / 2
                        );
                        ghostsReturnToSpawn[i] = false;
                    }
                }
                // Update ghost movement if not returning to spawn
                else if (!ghostsReturnToSpawn[i]) {
                    g->updateAutonomous(maze);

                    // Check for collision with Pacman
                    if (g->GhostCollision(pacman.GetPosition())) {
                        if (superMode) {
                            // In super mode, ghost gets eaten
                            score += 200;
                            ghostsBlinking[i] = true;
                            ghostBlinkTimers[i] = 0.0f;
                        }
                        else {
                            // Normal mode - Pacman loses a life
                            lives--;

                            // Start the life lost countdown
                            lifeLostCountdown = true;
                            lifeLostTimer = 0.0f;

                            // Reset ghost positions to their initial spawn positions
                            for (size_t j = 0; j < gameGhosts.size(); j++) {
                                // Get the appropriate ghost spawn position based on ghost index
                                Vector2i spawnPos;
                                char ghostId = '0' + j;  // Convert to ghost ID character ('0', '1', '2', '3')
                                if (j < 4) {
                                    spawnPos = maze.getGhost(ghostId);
                                }
                                else {
                                    spawnPos = maze.getGhost('0');  // Default to ghost 0's position if out of range
                                }

                                gameGhosts[j]->SetPosition(
                                    (spawnPos.x * cellSize + cellSize / 2) + 20,
                                    (spawnPos.y * cellSize + cellSize / 2) + 20
                                );

                                // Reset ghost state if needed
                                ghostsBlinking[j] = false;
                                ghostsReturnToSpawn[j] = false;
                                gameGhosts[j]->setColor(originalGhostColors[j]);  // Restore original color
                            }

                            // Reset Pacman position after losing a life
                            pacman.SetPosition(pacmanStartPos.x, pacmanStartPos.y);
                            break; // Exit ghost loop to prevent further processing
                        }
                    }
                }

                g->Update(dt);
                window.draw(g->getSprite());
            }

            // Check if all food has been eaten
            if (!(maze.foodremains())) {
                // Game won logic
                cout << "You Win!" << endl;
                // Return to menu
                gameOver = true;
                gameStarted = false;
                // Reset game state
                lives = 3;
                superMode = false;
                // Clean up ghosts
                for (auto ghost : gameGhosts) {
                    delete ghost;
                }
                gameGhosts.clear();
                // Reset maze
                maze.reset();
                // Reset Pacman position
                pacman.SetPosition(pacmanStartPos.x, pacmanStartPos.y);
                // Restart menu music
                playMenuMusic();
            }

            // Draw Pacman - only when in game mode
            window.draw(pacman.getSprite());

            // Draw UI elements (score, lives, etc.) - only when in game mode
            drawUI(window, font, score,highScore, lives, superMode, superModeTimer);
        }
        else if (gameOver) {
            // Display game over screen
            // Draw background

            if (score > highScore) {
                highScore = score;
                std::ofstream highScoreFileOut("highscore.txt");
                if (highScoreFileOut.is_open()) {
                    highScoreFileOut << highScore;
                    highScoreFileOut.close();
                }
            }
            for (auto& d : dots) {
                window.draw(d.shape);
            }

            Text gameOverText("GAME OVER", font, 70);
            gameOverText.setFillColor(Color::Red);
            gameOverText.setPosition(windowWidth / 2.f - gameOverText.getGlobalBounds().width / 2.f, 300);
            window.draw(gameOverText);

            Text Score("SCORE: " + to_string(score), font, 100);
            Score.setFillColor(Color::White);
            Score.setPosition((windowWidth / 2.f - gameOverText.getGlobalBounds().width / 2.f) - 100, 380);
            window.draw(Score);

            Text highScoreText("HIGH SCORE: " + to_string(highScore), font, 50);
            highScoreText.setFillColor(Color::Yellow);
            highScoreText.setPosition(windowWidth / 2.f - highScoreText.getGlobalBounds().width / 2.f, 550);
            window.draw(highScoreText);

            if (score == 0) {
                Text rem("That was intentional right ?", font, 40);
                rem.setFillColor(Color::White);
                rem.setOrigin(rem.getLocalBounds().left + rem.getLocalBounds().width / 2.f,
                    rem.getLocalBounds().top + rem.getLocalBounds().height / 2.f);
                rem.setPosition(windowWidth / 2.f, 520);
                window.draw(rem);
            }
            else if (score > 0 && score < 1000) {
                Text rem("You're getting there?", font, 40);
                rem.setFillColor(Color::White);
                rem.setOrigin(rem.getLocalBounds().left + rem.getLocalBounds().width / 2.f,
                    rem.getLocalBounds().top + rem.getLocalBounds().height / 2.f);
                rem.setPosition(windowWidth / 2.f, 520);
                window.draw(rem);
            }
            else if (score > 999 && score < 2000) {
                Text rem("Pretty Impressive huh", font, 50);
                rem.setFillColor(Color::White);
                rem.setOrigin(rem.getLocalBounds().left + rem.getLocalBounds().width / 2.f,
                    rem.getLocalBounds().top + rem.getLocalBounds().height / 2.f);
                rem.setPosition(windowWidth / 2.f, 520);
                window.draw(rem);
            }
            else if (score > 1999 && score < 3000 && maze.foodremains()) {
                Text rem("Almost Completed Huh", font, 50);
                rem.setFillColor(Color::White);
                rem.setOrigin(rem.getLocalBounds().left + rem.getLocalBounds().width / 2.f,
                    rem.getLocalBounds().top + rem.getLocalBounds().height / 2.f);
                rem.setPosition(windowWidth / 2.f, 520);
                window.draw(rem);
            }
            else if (score > 1999 && score < 3000 && !maze.foodremains()) {
                Text rem("COMPLETED LESSGOO", font, 80);
                rem.setFillColor(Color::White);
                rem.setOrigin(rem.getLocalBounds().left + rem.getLocalBounds().width / 2.f,
                    rem.getLocalBounds().top + rem.getLocalBounds().height / 2.f);
                rem.setPosition(windowWidth / 2.f, 520);
                window.draw(rem);
            }
            else if (score > 2999 && score < 4000 && !maze.foodremains()) {
                Text rem("COMPLETED LESSGOO", font, 80);
                rem.setFillColor(Color::White);
                rem.setOrigin(rem.getLocalBounds().left + rem.getLocalBounds().width / 2.f,
                    rem.getLocalBounds().top + rem.getLocalBounds().height / 2.f);
                rem.setPosition(windowWidth / 2.f, 520);
                window.draw(rem);
            }
            else if (score > 2999 && score < 4000 && maze.foodremains()) {
                Text rem("How'd you not win?", font, 40);
                rem.setFillColor(Color::White);
                rem.setOrigin(rem.getLocalBounds().left + rem.getLocalBounds().width / 2.f,
                    rem.getLocalBounds().top + rem.getLocalBounds().height / 2.f);
                rem.setPosition(windowWidth / 2.f, 520);
                window.draw(rem);
            }
            else if (score > 4000 && !maze.foodremains()) {
                Text rem("HOW DID YOU GET 4K+????", font, 40);
                rem.setFillColor(Color::White);
                rem.setOrigin(rem.getLocalBounds().left + rem.getLocalBounds().width / 2.f,
                    rem.getLocalBounds().top + rem.getLocalBounds().height / 2.f);
                rem.setPosition(windowWidth / 2.f, 520);
                window.draw(rem);
            }

            static float flashTimer = 0.0f;
            flashTimer += dt;
            if (sin(flashTimer * 3.0f) > 0) {  // Flash at 3Hz
                Text pressEnter;
                pressEnter.setString("PRESS ENTER TO RETURN TO MENU");
                pressEnter.setFont(*menuTexts[0].getFont());
                pressEnter.setCharacterSize(30);
                pressEnter.setFillColor(Color::White);

                // Center the text
                pressEnter.setPosition(
                    windowWidth / 2.f - pressEnter.getGlobalBounds().width / 2.f,
                    650
                );

                // Add a shadow for better visibility
                Text shadowPressEnter = pressEnter;
                shadowPressEnter.setFillColor(Color(30, 30, 30, 150));
                shadowPressEnter.setPosition(pressEnter.getPosition() + Vector2f(2, 2));
                window.draw(shadowPressEnter);
                window.draw(pressEnter);
            }
        }

        window.display();
    }
}





int main() {
    MainGame();
    return 0;
}