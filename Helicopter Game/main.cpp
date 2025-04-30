#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <string>
#include <cctype>
#include <vector>
#include <random>
#include <ctime>
#include <fstream>
#include <algorithm>

// ======================
// Game Constants (Organized)
// ======================
namespace Constants {
    // Window
    constexpr int WINDOW_WIDTH = 800;
    constexpr int WINDOW_HEIGHT = 600;

    // Physics
    constexpr float LANDING_HEIGHT = 70.f;
    constexpr float GRAVITY = 90.f;
    constexpr float HELI_SCALE = 0.01f;

    // Audio
    constexpr float MENU_MUSIC_VOLUME = 50.f;
    constexpr float GAME_MUSIC_VOLUME = 60.f;
    constexpr float SOUND_EFFECT_VOLUME = 70.f;

    // Fuel
    constexpr float MAX_FUEL = 100.f;
    constexpr float FUEL_REGEN_RATE = 2.5f;
    constexpr float FUEL_BOTTLE_VALUE = 30.f;

    // Spawn Rates
    constexpr float COIN5_SPAWN_RATE = 2.0f;
    constexpr float COIN10_SPAWN_RATE = 6.0f;
    constexpr float COIN50_SPAWN_RATE = 15.0f;
    constexpr float FUEL_BOTTLE_SPAWN_RATE = 9.0f;

    // Difficulty
    namespace Easy {
        constexpr float SCROLL_SPEED = 80.f;
        constexpr float FUEL_CONSUMPTION = 2.0f;
        constexpr float OBSTACLE_SPAWN_RATE = 3.0f;
        constexpr float MOVE_SPEED = 350.f;
    }

    namespace Medium {
        constexpr float SCROLL_SPEED = 100.f;
        constexpr float FUEL_CONSUMPTION = 2.5f;
        constexpr float OBSTACLE_SPAWN_RATE = 2.5f;
        constexpr float MOVE_SPEED = 400.f;
    }

    namespace Hard {
        constexpr float SCROLL_SPEED = 130.f;
        constexpr float FUEL_CONSUMPTION = 3.0f;
        constexpr float OBSTACLE_SPAWN_RATE = 1.5f;
        constexpr float MOVE_SPEED = 450.f;
    }

    // Birds
    constexpr float BIRD_SPAWN_CHANCE = 0.9f;
    constexpr float BIRD_MIN_SPEED_MULTIPLIER = 1.6f;
    constexpr float BIRD_MAX_SPEED_MULTIPLIER = 2.2f;
    constexpr float BIRD_VERTICAL_SPEED_RANGE = 150.f;

    // Paths
    const std::string HIGHSCORE_FILE = "highscores.txt";
    const std::string FONT_PATH = "E:/Helicopter Game/Helicopter Game/Helicopter Game/Assets/Fonts/bruce.ttf";
    const std::string MENU_BG_PATH = "E:/Helicopter Game/Helicopter Game/Helicopter Game/Assets/Images/menu.jpg";
    const std::string BG_PATH = "E:/Helicopter Game/Helicopter Game/Helicopter Game/Assets/Images/background.jpg";
    const std::string HELI_PATH = "E:/Helicopter Game/Helicopter Game/Helicopter Game/Assets/Images/helicopter.png";
    const std::string BIRD_PATH = "E:/Helicopter Game/Helicopter Game/Helicopter Game/Assets/Images/bird.png";
    const std::string TREE_PATH = "E:/Helicopter Game/Helicopter Game/Helicopter Game/Assets/Images/tree.png";
    const std::string COIN5_PATH = "E:/Helicopter Game/Helicopter Game/Helicopter Game/Assets/Images/coin5.png";
    const std::string COIN10_PATH = "E:/Helicopter Game/Helicopter Game/Helicopter Game/Assets/Images/coin10.png";
    const std::string COIN50_PATH = "E:/Helicopter Game/Helicopter Game/Helicopter Game/Assets/Images/coin50.png";
    const std::string FUEL_PATH = "E:/Helicopter Game/Helicopter Game/Helicopter Game/Assets/Images/fuel_bottle.png";
    const std::string CLICK_SOUND = "E:/Helicopter Game/Helicopter Game/Helicopter Game/Assets/Sounds/click.wav";
    const std::string ENGINE_SOUND = "E:/Helicopter Game/Helicopter Game/Helicopter Game/Assets/Sounds/engine.wav";
    const std::string CRASH_SOUND = "E:/Helicopter Game/Helicopter Game/Helicopter Game/Assets/Sounds/crash.wav";
    const std::string COIN_SOUND = "E:/Helicopter Game/Helicopter Game/Helicopter Game/Assets/Sounds/coin.wav";
    const std::string FUEL_SOUND = "E:/Helicopter Game/Helicopter Game/Helicopter Game/Assets/Sounds/fuel.wav";
    const std::string MENU_MUSIC = "E:/Helicopter Game/Helicopter Game/Helicopter Game/Assets/Sounds/menu_music.ogg";
    const std::string GAME_MUSIC = "E:/Helicopter Game/Helicopter Game/Helicopter Game/Assets/Sounds/game_music.ogg";
}

// ======================
// Game State & Difficulty Enums
// ======================
enum class GameState {
    Menu,
    NameInput,
    DifficultySelect,
    Playing,
    GameOver,
    Options,
    Help,
    Settings,
    Credits,
    Paused,
    HighScores
};

enum class Difficulty {
    Easy,
    Medium,
    Hard
};

enum class ObstacleType {
    Bird,
    Tree
};

enum class CoinType {
    Coin5,
    Coin10,
    Coin50
};

// ======================
// HighScoreEntry with stable sorting
// ======================
struct HighScoreEntry {
    std::string name;
    int score;
    Difficulty difficulty;

    bool operator<(const HighScoreEntry& other) const {
        if (score != other.score) return score > other.score;
        return name < other.name; // Secondary sort by name
    }
};

// ======================
// Resource Manager
// ======================
class ResourceManager {
public:
    static bool loadFont(sf::Font& font, const std::string& path) {
        if (!font.loadFromFile(path)) {
            std::cerr << "ERROR: Failed to load font from " << path << std::endl;
            return false;
        }
        return true;
    }

    static bool loadTexture(sf::Texture& texture, const std::string& path) {
        if (!texture.loadFromFile(path)) {
            std::cerr << "ERROR: Failed to load texture from " << path << std::endl;

            // Create error placeholder
            sf::Image placeholder;
            placeholder.create(64, 64, sf::Color::Magenta);
            for (int i = 0; i < 64; i++) {
                placeholder.setPixel(i, i, sf::Color::White);
                placeholder.setPixel(63 - i, i, sf::Color::White);
            }

            if (!texture.loadFromImage(placeholder)) {
                std::cerr << "FATAL: Failed to create placeholder texture" << std::endl;
                return false;
            }
            return false;
        }
        return true;
    }

    static bool loadSound(sf::SoundBuffer& buffer, sf::Sound& sound, const std::string& path) {
        if (!buffer.loadFromFile(path)) {
            std::cerr << "ERROR: Failed to load sound from " << path << std::endl;
            return false;
        }
        sound.setBuffer(buffer);
        sound.setVolume(Constants::SOUND_EFFECT_VOLUME);
        return true;
    }

    static bool loadMusic(sf::Music& music, const std::string& path) {
        if (!music.openFromFile(path)) {
            std::cerr << "ERROR: Failed to load music from " << path << std::endl;
            return false;
        }
        return true;
    }
};

// ======================
// Game Objects
// ======================
class FuelBottle {
public:
    FuelBottle(const sf::Texture& texture, float x, float y)
        : sprite(texture), active(true) {
        sprite.setPosition(x, y);
        sprite.setScale(0.01f, 0.01f);
    }

    void update(float deltaTime) {}

    void draw(sf::RenderWindow& window) const {
        if (active) {
            window.draw(sprite);
        }
    }

    bool isActive() const { return active; }
    const sf::FloatRect getBounds() const { return sprite.getGlobalBounds(); }
    sf::Sprite& getSprite() { return sprite; }
    void deactivate() { active = false; }

private:
    sf::Sprite sprite;
    bool active;
};

class Coin {
public:
    Coin(CoinType type, const sf::Texture& texture, float x, float y)
        : type(type), sprite(texture), active(true) {
        sprite.setPosition(x, y);

        switch (type) {
        case CoinType::Coin5:
            sprite.setScale(0.06f, 0.06f);
            break;
        case CoinType::Coin10:
            sprite.setScale(0.09f, 0.09f);
            break;
        case CoinType::Coin50:
            sprite.setScale(0.12f, 0.12f);
            break;
        }
    }

    void update(float deltaTime) {}

    void draw(sf::RenderWindow& window) const {
        if (active) {
            window.draw(sprite);
        }
    }

    bool isActive() const { return active; }
    const sf::FloatRect getBounds() const { return sprite.getGlobalBounds(); }
    sf::Sprite& getSprite() { return sprite; }
    CoinType getType() const { return type; }
    int getValue() const {
        switch (type) {
        case CoinType::Coin5: return 5;
        case CoinType::Coin10: return 10;
        case CoinType::Coin50: return 50;
        }
        return 0;
    }

    void deactivate() { active = false; }

private:
    CoinType type;
    sf::Sprite sprite;
    bool active;
};

class Obstacle {
public:
    Obstacle(ObstacleType type, const sf::Texture& texture, float x, float y, float speed)
        : type(type), sprite(texture), speed(speed), active(true),
        verticalSpeed(0.f), movementPatternTime(0.f) {
        sprite.setPosition(x, y);

        switch (type) {
        case ObstacleType::Bird:
            sprite.setScale(0.05f, 0.05f);
            verticalSpeed = (rand() % static_cast<int>(Constants::BIRD_VERTICAL_SPEED_RANGE * 2)) - Constants::BIRD_VERTICAL_SPEED_RANGE;
            movementPatternDuration = 0.5f + (rand() % 100) / 100.0f;
            break;
        case ObstacleType::Tree:
            sprite.setScale(0.04f, 0.04f);
            break;
        }
    }

    void update(float deltaTime, bool isLanded, float scrollSpeed) {
        movementPatternTime += deltaTime;

        if (type == ObstacleType::Bird) {
            if (movementPatternTime >= movementPatternDuration) {
                movementPatternTime = 0.f;
                verticalSpeed = (rand() % static_cast<int>(Constants::BIRD_VERTICAL_SPEED_RANGE * 2)) - Constants::BIRD_VERTICAL_SPEED_RANGE;
                movementPatternDuration = 0.3f + (rand() % 70) / 100.0f;
            }

            sprite.move(-speed * deltaTime, verticalSpeed * deltaTime);

            sf::Vector2f pos = sprite.getPosition();
            if (pos.y < 0) pos.y = 0;
            if (pos.y > Constants::WINDOW_HEIGHT - sprite.getGlobalBounds().height)
                pos.y = Constants::WINDOW_HEIGHT - sprite.getGlobalBounds().height;
            sprite.setPosition(pos);
        }
        else if (type == ObstacleType::Tree && !isLanded) {
            sprite.move(-scrollSpeed * deltaTime, 0);
        }

        if (sprite.getPosition().x + sprite.getGlobalBounds().width < 0) {
            active = false;
        }
    }

    void draw(sf::RenderWindow& window) const {
        window.draw(sprite);
    }

    bool isActive() const { return active; }
    const sf::FloatRect getBounds() const { return sprite.getGlobalBounds(); }
    ObstacleType getType() const { return type; }

private:
    ObstacleType type;
    sf::Sprite sprite;
    float speed;
    bool active;
    float verticalSpeed;
    float movementPatternTime;
    float movementPatternDuration;
};

class Button {
public:
    Button(const std::string& text, const sf::Font& font, unsigned int characterSize,
        const sf::Color& textColor, const sf::Color& buttonColor,
        const sf::Vector2f& position, const sf::Vector2f& size,
        sf::Sound* clickSound = nullptr)
        : m_shape(size), m_text(text, font, characterSize),
        m_normalColor(buttonColor), m_hoverColor(sf::Color(0, 100, 0)),
        m_normalTextColor(textColor), m_hoverTextColor(sf::Color::Black),
        m_clickSound(clickSound) {

        m_shape.setPosition(position);
        m_shape.setFillColor(buttonColor);
        m_shape.setOutlineThickness(2.f);
        m_shape.setOutlineColor(sf::Color(220, 220, 220));

        sf::FloatRect textRect = m_text.getLocalBounds();
        m_text.setOrigin(textRect.left + textRect.width / 2.0f,
            textRect.top + textRect.height / 2.0f);
        m_text.setPosition(position.x + size.x / 2.0f, position.y + size.y / 2.0f);
        m_text.setFillColor(textColor);

        m_bounds = sf::FloatRect(position, size);
    }

    void draw(sf::RenderWindow& window) const {
        window.draw(m_shape);
        window.draw(m_text);
    }

    bool isMouseOver(const sf::RenderWindow& window) const {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        return m_bounds.contains(mousePos);
    }

    void setHighlight(bool highlight) {
        if (highlight) {
            m_shape.setFillColor(m_hoverColor);
            m_text.setFillColor(m_hoverTextColor);
            m_shape.setOutlineColor(sf::Color::Black);
            m_shape.setOutlineThickness(3.f);
        }
        else {
            m_shape.setFillColor(m_normalColor);
            m_text.setFillColor(m_normalTextColor);
            m_shape.setOutlineColor(sf::Color(220, 220, 220));
            m_shape.setOutlineThickness(2.f);
        }
    }

    void playClickSound() {
        if (m_clickSound) {
            m_clickSound->play();
        }
    }

private:
    sf::RectangleShape m_shape;
    sf::Text m_text;
    sf::FloatRect m_bounds;
    sf::Color m_normalColor;
    sf::Color m_hoverColor;
    sf::Color m_normalTextColor;
    sf::Color m_hoverTextColor;
    sf::Sound* m_clickSound;
};

// ======================
// Main Game Class
// ======================
class HelicopterGame {
private:
    sf::RenderWindow window;
    GameState currentState;
    Difficulty currentDifficulty;
    std::string playerName;
    std::vector<HighScoreEntry> highScores;

    float currentScrollSpeed;
    float currentFuelConsumption;
    float currentObstacleSpawnRate;
    float currentMoveSpeed;

    // Resources
    sf::Texture menuBgTexture;
    sf::RectangleShape menuBackground;
    sf::Font font;

    // Sound buffers (must persist)
    sf::SoundBuffer clickBuffer;
    sf::SoundBuffer engineBuffer;
    sf::SoundBuffer crashBuffer;
    sf::SoundBuffer coinBuffer;
    sf::SoundBuffer fuelBuffer;

    // Sounds
    sf::Sound clickSound;
    sf::Sound engineSound;
    sf::Sound crashSound;
    sf::Sound coinSound;
    sf::Sound fuelSound;

    // Music
    sf::Music bgMusic;
    sf::Music gameMusic;

    // Textures
    sf::Texture bgTexture;
    sf::Sprite bgSprites[2];
    sf::Texture heliTexture;
    sf::Sprite helicopter;
    sf::Texture birdTexture;
    sf::Texture treeTexture;
    sf::Texture coin5Texture;
    sf::Texture coin10Texture;
    sf::Texture coin50Texture;
    sf::Texture fuelBottleTexture;

    // Game state
    bool isLanded;
    bool gameStarted;
    bool gameOver;
    sf::Clock gameClock;
    sf::Clock obstacleClock;
    sf::Clock coin5Clock;
    sf::Clock coin10Clock;
    sf::Clock coin50Clock;
    sf::Clock fuelBottleClock;
    sf::Clock fuelClock;
    float obstacleSpawnTimer;
    int score;
    float fuel;

    // Game objects
    std::vector<Obstacle> obstacles;
    std::vector<Coin> coins;
    std::vector<FuelBottle> fuelBottles;

    // UI elements
    sf::RectangleShape fuelBackground;
    sf::RectangleShape fuelBar;
    sf::Text fuelText;
    sf::Text namePrompt;
    sf::RectangleShape nameInputBox;
    sf::Text nameInputText;
    Button nameSubmitButton;
    Button playButton;
    Button optionsButton;
    Button creditsButton;
    Button exitButton;
    Button helpButton;
    Button settingsButton;
    Button backButton;
    Button restartButton;
    Button gameOverBackButton;
    Button resumeButton;
    Button pauseQuitButton;
    Button easyButton;
    Button mediumButton;
    Button hardButton;
    Button highScoresButton;

    // Random number generation
    std::mt19937 rng;
    std::uniform_int_distribution<int> obstacleDist;
    std::uniform_int_distribution<int> heightDist;

    // Helper functions
    static Button createMenuButton(const std::string& text, const sf::Font& font,
        float yPos, sf::Vector2f windowSize, sf::Sound* clickSound,
        sf::Color buttonColor = sf::Color(46, 125, 50, 200)) {
        const sf::Vector2f buttonSize(200.f, 50.f);
        return Button(text, font, 24, sf::Color::White, buttonColor,
            sf::Vector2f((windowSize.x - buttonSize.x) / 2.0f, yPos), buttonSize, clickSound);
    }

    void loadResources() {
        // Initialize random
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        rng = std::mt19937(std::rand());
        obstacleDist = std::uniform_int_distribution<int>(0, 1);
        heightDist = std::uniform_int_distribution<int>(100, 400);

        // Load font
        if (!ResourceManager::loadFont(font, Constants::FONT_PATH)) {
            // Try to continue with default font
            if (!font.loadFromFile("arial.ttf")) {
                std::cerr << "FATAL: No font available!" << std::endl;
            }
        }

        // Load menu background
        if (!ResourceManager::loadTexture(menuBgTexture, Constants::MENU_BG_PATH)) {
            menuBackground.setSize(sf::Vector2f(window.getSize()));
            menuBackground.setFillColor(sf::Color(30, 30, 60));
        }
        else {
            menuBackground.setTexture(&menuBgTexture);
            menuBackground.setSize(sf::Vector2f(window.getSize()));
        }

        // Load sounds
        ResourceManager::loadSound(clickBuffer, clickSound, Constants::CLICK_SOUND);
        ResourceManager::loadSound(engineBuffer, engineSound, Constants::ENGINE_SOUND);
        ResourceManager::loadSound(crashBuffer, crashSound, Constants::CRASH_SOUND);
        ResourceManager::loadSound(coinBuffer, coinSound, Constants::COIN_SOUND);
        ResourceManager::loadSound(fuelBuffer, fuelSound, Constants::FUEL_SOUND);
        engineSound.setLoop(true);

        // Load music
        if (ResourceManager::loadMusic(bgMusic, Constants::MENU_MUSIC)) {
            bgMusic.setLoop(true);
            bgMusic.setVolume(Constants::MENU_MUSIC_VOLUME);
            bgMusic.play();
        }
        if (ResourceManager::loadMusic(gameMusic, Constants::GAME_MUSIC)) {
            gameMusic.setLoop(true);
            gameMusic.setVolume(Constants::GAME_MUSIC_VOLUME);
        }

        // Load textures
        ResourceManager::loadTexture(bgTexture, Constants::BG_PATH);
        ResourceManager::loadTexture(heliTexture, Constants::HELI_PATH);
        ResourceManager::loadTexture(birdTexture, Constants::BIRD_PATH);
        ResourceManager::loadTexture(treeTexture, Constants::TREE_PATH);
        ResourceManager::loadTexture(coin5Texture, Constants::COIN5_PATH);
        ResourceManager::loadTexture(coin10Texture, Constants::COIN10_PATH);
        ResourceManager::loadTexture(coin50Texture, Constants::COIN50_PATH);
        ResourceManager::loadTexture(fuelBottleTexture, Constants::FUEL_PATH);

        // Setup background sprites
        float scaleX = static_cast<float>(Constants::WINDOW_WIDTH) / bgTexture.getSize().x;
        float scaleY = static_cast<float>(Constants::WINDOW_HEIGHT) / bgTexture.getSize().y;
        for (int i = 0; i < 2; ++i) {
            bgSprites[i].setTexture(bgTexture);
            bgSprites[i].setScale(scaleX, scaleY);
            bgSprites[i].setPosition(i * static_cast<float>(Constants::WINDOW_WIDTH), 0.f);
        }

        // Setup helicopter
        helicopter.setTexture(heliTexture);
        helicopter.setScale(Constants::HELI_SCALE, Constants::HELI_SCALE);
        helicopter.setOrigin(heliTexture.getSize().x / 2.0f, heliTexture.getSize().y / 2.0f);
        helicopter.setPosition(Constants::WINDOW_WIDTH / 4.0f, Constants::WINDOW_HEIGHT / 2.0f);

        // Setup fuel UI
        fuelBackground.setSize(sf::Vector2f(104.f, 24.f));
        fuelBackground.setFillColor(sf::Color(50, 50, 50));
        fuelBackground.setOutlineThickness(2.f);
        fuelBackground.setOutlineColor(sf::Color::White);
        fuelBackground.setPosition(Constants::WINDOW_WIDTH - 120.f, 20.f);

        fuelBar.setSize(sf::Vector2f(100.f, 20.f));
        fuelBar.setFillColor(sf::Color::Green);
        fuelBar.setPosition(Constants::WINDOW_WIDTH - 118.f, 22.f);

        fuelText.setFont(font);
        fuelText.setCharacterSize(16);
        fuelText.setFillColor(sf::Color::White);
        fuelText.setPosition(Constants::WINDOW_WIDTH - 118.f, 22.f);

        // Setup buttons
        const sf::Vector2f windowSize = static_cast<sf::Vector2f>(window.getSize());
        const float startY = 200.f;

        playButton = createMenuButton("Play", font, startY, windowSize, &clickSound);
        optionsButton = createMenuButton("Options", font, startY + 70.f, windowSize, &clickSound, sf::Color(33, 150, 243, 200));
        highScoresButton = createMenuButton("Scores", font, startY + 140.f, windowSize, &clickSound, sf::Color(255, 193, 7, 200));
        creditsButton = createMenuButton("Credits", font, startY + 210.f, windowSize, &clickSound, sf::Color(156, 39, 176, 200));
        exitButton = createMenuButton("Exit Game", font, startY + 280.f, windowSize, &clickSound, sf::Color(211, 47, 47, 200));

        helpButton = createMenuButton("Help", font, 250.f, windowSize, &clickSound, sf::Color(33, 150, 243, 200));
        settingsButton = createMenuButton("Settings", font, 330.f, windowSize, &clickSound, sf::Color(156, 39, 176, 200));
        backButton = createMenuButton("Back", font, 410.f, windowSize, &clickSound, sf::Color(211, 47, 47, 200));

        restartButton = createMenuButton("Restart", font, 300.f, windowSize, &clickSound);
        gameOverBackButton = createMenuButton("Back", font, 380.f, windowSize, &clickSound, sf::Color(211, 47, 47, 200));

        resumeButton = Button("Resume", font, 24, sf::Color::White, sf::Color(46, 125, 50, 200),
            sf::Vector2f((windowSize.x - 200.f) / 2.0f, 250.f),
            sf::Vector2f(200.f, 50.f), &clickSound);
        pauseQuitButton = Button("Quit", font, 24, sf::Color::White, sf::Color(211, 47, 47, 200),
            sf::Vector2f((windowSize.x - 200.f) / 2.0f, 320.f),
            sf::Vector2f(200.f, 50.f), &clickSound);

        easyButton = Button("Easy", font, 24, sf::Color::White, sf::Color(100, 221, 23, 200),
            sf::Vector2f((windowSize.x - 200.f) / 2.0f, 200.f),
            sf::Vector2f(200.f, 50.f), &clickSound);
        mediumButton = Button("Medium", font, 24, sf::Color::White, sf::Color(255, 204, 0, 200),
            sf::Vector2f((windowSize.x - 200.f) / 2.0f, 270.f),
            sf::Vector2f(200.f, 50.f), &clickSound);
        hardButton = Button("Hard", font, 24, sf::Color::White, sf::Color(255, 71, 26, 200),
            sf::Vector2f((windowSize.x - 200.f) / 2.0f, 340.f),
            sf::Vector2f(200.f, 50.f), &clickSound);

        // Name input setup
        namePrompt.setFont(font);
        namePrompt.setString("Enter your name:");
        namePrompt.setCharacterSize(30);
        namePrompt.setFillColor(sf::Color::White);
        namePrompt.setPosition((windowSize.x - namePrompt.getLocalBounds().width) / 2.0f, 200.f);

        nameInputBox.setSize(sf::Vector2f(400.f, 50.f));
        nameInputBox.setFillColor(sf::Color(70, 70, 70, 200));
        nameInputBox.setOutlineThickness(2.f);
        nameInputBox.setOutlineColor(sf::Color::White);
        nameInputBox.setPosition((windowSize.x - nameInputBox.getSize().x) / 2.0f, 250.f);

        nameInputText.setFont(font);
        nameInputText.setCharacterSize(28);
        nameInputText.setFillColor(sf::Color::White);
        nameInputText.setPosition(nameInputBox.getPosition().x + nameInputBox.getSize().x / 2.f,
            nameInputBox.getPosition().y + nameInputBox.getSize().y / 2.f);
        nameInputText.setOrigin(nameInputText.getLocalBounds().width / 2.f,
            nameInputText.getLocalBounds().height / 2.f + 5.f);

        nameSubmitButton = Button("Continue", font, 24, sf::Color::White, sf::Color(46, 125, 50, 200),
            sf::Vector2f((windowSize.x - 180.f) / 2.0f, 320.f),
            sf::Vector2f(180.f, 45.f), &clickSound);

        // Set default difficulty
        currentDifficulty = Difficulty::Medium;
        currentScrollSpeed = Constants::Medium::SCROLL_SPEED;
        currentFuelConsumption = Constants::Medium::FUEL_CONSUMPTION;
        currentObstacleSpawnRate = Constants::Medium::OBSTACLE_SPAWN_RATE;
        currentMoveSpeed = Constants::Medium::MOVE_SPEED;

        // Load high scores
        loadHighScores();
    }

    void loadHighScores() {
        highScores.clear();

        // Create file if it doesn't exist
        std::ifstream file(Constants::HIGHSCORE_FILE);
        if (!file.is_open()) {
            std::ofstream createFile(Constants::HIGHSCORE_FILE);
            if (!createFile) {
                std::cerr << "WARNING: Could not create high scores file" << std::endl;
                return;
            }
            createFile.close();
            file.open(Constants::HIGHSCORE_FILE);
        }

        // Read scores
        HighScoreEntry entry;
        std::string line;
        while (std::getline(file, line)) {
            size_t firstComma = line.find(',');
            size_t secondComma = line.find(',', firstComma + 1);

            if (firstComma != std::string::npos && secondComma != std::string::npos) {
                entry.name = line.substr(0, firstComma);
                try {
                    entry.score = std::stoi(line.substr(firstComma + 1, secondComma - firstComma - 1));
                    int diff = std::stoi(line.substr(secondComma + 1));
                    entry.difficulty = static_cast<Difficulty>(diff);
                    highScores.push_back(entry);
                }
                catch (...) {
                    std::cerr << "WARNING: Invalid high score entry: " << line << std::endl;
                }
            }
        }

        std::sort(highScores.begin(), highScores.end());
    }

    void saveHighScores() {
        std::ofstream file(Constants::HIGHSCORE_FILE);
        if (file.is_open()) {
            for (const auto& entry : highScores) {
                file << entry.name << "," << entry.score << ","
                    << static_cast<int>(entry.difficulty) << "\n";
            }
        }
        else {
            std::cerr << "ERROR: Could not save high scores" << std::endl;
        }
    }

    void addHighScore(const std::string& name, int score, Difficulty difficulty) {
        HighScoreEntry entry{ name, score, difficulty };
        highScores.push_back(entry);
        std::sort(highScores.begin(), highScores.end());
        if (highScores.size() > 10) {
            highScores.resize(10);
        }
        saveHighScores();
    }

    void spawnObstacle() {
        ObstacleType type = (static_cast<float>(rand()) / RAND_MAX < Constants::BIRD_SPAWN_CHANCE) ?
            ObstacleType::Bird : ObstacleType::Tree;

        float height;
        if (type == ObstacleType::Bird) {
            height = rand() % (Constants::WINDOW_HEIGHT - 100);
        }
        else {
            height = Constants::WINDOW_HEIGHT - Constants::LANDING_HEIGHT - treeTexture.getSize().y * 0.04f;
        }

        float speed;
        if (type == ObstacleType::Tree) {
            speed = currentScrollSpeed;
        }
        else {
            float speedMultiplier = Constants::BIRD_MIN_SPEED_MULTIPLIER +
                (static_cast<float>(rand()) / RAND_MAX) * (Constants::BIRD_MAX_SPEED_MULTIPLIER - Constants::BIRD_MIN_SPEED_MULTIPLIER);
            speed = currentScrollSpeed * speedMultiplier;
        }

        sf::Texture* texture = (type == ObstacleType::Bird) ? &birdTexture : &treeTexture;
        obstacles.emplace_back(type, *texture, static_cast<float>(Constants::WINDOW_WIDTH), height, speed);
    }

    void spawnCoin(CoinType type) {
        float x = static_cast<float>(Constants::WINDOW_WIDTH);
        float y = 50.f + static_cast<float>(rand() % (Constants::WINDOW_HEIGHT - 150));

        sf::Texture* texture = nullptr;
        switch (type) {
        case CoinType::Coin5: texture = &coin5Texture; break;
        case CoinType::Coin10: texture = &coin10Texture; break;
        case CoinType::Coin50: texture = &coin50Texture; break;
        }

        if (texture) {
            coins.emplace_back(type, *texture, x, y);
        }
    }

    void spawnFuelBottle() {
        float x = static_cast<float>(Constants::WINDOW_WIDTH);
        float y = 50.f + static_cast<float>(rand() % (Constants::WINDOW_HEIGHT - 150));
        fuelBottles.emplace_back(fuelBottleTexture, x, y);
    }

    void updateFuel(float deltaTime) {
        if (isLanded) {
            fuel += Constants::FUEL_REGEN_RATE * deltaTime;
            if (fuel > Constants::MAX_FUEL) fuel = Constants::MAX_FUEL;
        }
        else {
            fuel -= currentFuelConsumption * deltaTime;
            if (fuel <= 0) {
                fuel = 0;
                gameOver = true;
                gameOverState();
            }
        }

        fuelBar.setSize(sf::Vector2f(fuel, 20.f));

        if (fuel > 50) {
            fuelBar.setFillColor(sf::Color::Green);
        }
        else if (fuel > 20) {
            fuelBar.setFillColor(sf::Color::Yellow);
        }
        else {
            fuelBar.setFillColor(sf::Color::Red);
        }

        fuelText.setString(std::to_string(static_cast<int>(fuel)) + "%");
    }

    void updateCoins(float deltaTime) {
        if (coin5Clock.getElapsedTime().asSeconds() >= Constants::COIN5_SPAWN_RATE) {
            spawnCoin(CoinType::Coin5);
            coin5Clock.restart();
        }

        if (coin10Clock.getElapsedTime().asSeconds() >= Constants::COIN10_SPAWN_RATE) {
            spawnCoin(CoinType::Coin10);
            coin10Clock.restart();
        }

        if (coin50Clock.getElapsedTime().asSeconds() >= Constants::COIN50_SPAWN_RATE) {
            spawnCoin(CoinType::Coin50);
            coin50Clock.restart();
        }

        for (auto& coin : coins) {
            coin.update(deltaTime);
            if (coin.isActive() && helicopter.getGlobalBounds().intersects(coin.getBounds())) {
                coinSound.play();
                score += coin.getValue();
                coin.deactivate();
            }
        }

        coins.erase(std::remove_if(coins.begin(), coins.end(),
            [](const Coin& c) { return !c.isActive(); }),
            coins.end());
    }

    void updateFuelBottles(float deltaTime) {
        if (fuelBottleClock.getElapsedTime().asSeconds() >= Constants::FUEL_BOTTLE_SPAWN_RATE) {
            spawnFuelBottle();
            fuelBottleClock.restart();
        }

        for (auto& bottle : fuelBottles) {
            bottle.update(deltaTime);
            if (bottle.isActive() && helicopter.getGlobalBounds().intersects(bottle.getBounds())) {
                fuelSound.play();
                fuel = std::min(fuel + Constants::FUEL_BOTTLE_VALUE, Constants::MAX_FUEL);
                bottle.deactivate();
            }
        }

        fuelBottles.erase(std::remove_if(fuelBottles.begin(), fuelBottles.end(),
            [](const FuelBottle& b) { return !b.isActive(); }),
            fuelBottles.end());
    }

    void handleMenuInput() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                if (playButton.isMouseOver(window)) {
                    playButton.playClickSound();
                    currentState = GameState::NameInput;
                    playerName.clear();
                    nameInputText.setString("");
                }
                else if (optionsButton.isMouseOver(window)) {
                    optionsButton.playClickSound();
                    currentState = GameState::Options;
                }
                else if (highScoresButton.isMouseOver(window)) {
                    highScoresButton.playClickSound();
                    currentState = GameState::HighScores;
                }
                else if (creditsButton.isMouseOver(window)) {
                    creditsButton.playClickSound();
                    currentState = GameState::Credits;
                }
                else if (exitButton.isMouseOver(window)) {
                    exitButton.playClickSound();
                    window.close();
                }
            }
        }

        playButton.setHighlight(playButton.isMouseOver(window));
        optionsButton.setHighlight(optionsButton.isMouseOver(window));
        highScoresButton.setHighlight(highScoresButton.isMouseOver(window));
        creditsButton.setHighlight(creditsButton.isMouseOver(window));
        exitButton.setHighlight(exitButton.isMouseOver(window));
    }

    void handleNameInput() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                currentState = GameState::Menu;
                return;
            }

            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b' && !playerName.empty()) {
                    playerName.pop_back();
                }
                else if (event.text.unicode < 128 && event.text.unicode != '\r' &&
                    event.text.unicode != '\t' && playerName.length() < 15) {
                    // Only allow alphanumeric and space
                    char c = static_cast<char>(event.text.unicode);
                    if (isalnum(c) || c == ' ') {
                        playerName += c;
                    }
                }
                nameInputText.setString(playerName);
                nameInputText.setOrigin(nameInputText.getLocalBounds().width / 2.f,
                    nameInputText.getLocalBounds().height / 2.f + 5.f);
            }

            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                if (nameSubmitButton.isMouseOver(window)) {
                    nameSubmitButton.playClickSound();
                    if (playerName.length() >= 3) {
                        currentState = GameState::DifficultySelect;
                    }
                }
            }
        }

        nameSubmitButton.setHighlight(nameSubmitButton.isMouseOver(window));
    }

    void handleDifficultyInput() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                currentState = GameState::NameInput;
                return;
            }

            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                if (easyButton.isMouseOver(window)) {
                    easyButton.playClickSound();
                    currentDifficulty = Difficulty::Easy;
                    currentScrollSpeed = Constants::Easy::SCROLL_SPEED;
                    currentFuelConsumption = Constants::Easy::FUEL_CONSUMPTION;
                    currentObstacleSpawnRate = Constants::Easy::OBSTACLE_SPAWN_RATE;
                    currentMoveSpeed = Constants::Easy::MOVE_SPEED;
                    startGame();
                }
                else if (mediumButton.isMouseOver(window)) {
                    mediumButton.playClickSound();
                    currentDifficulty = Difficulty::Medium;
                    currentScrollSpeed = Constants::Medium::SCROLL_SPEED;
                    currentFuelConsumption = Constants::Medium::FUEL_CONSUMPTION;
                    currentObstacleSpawnRate = Constants::Medium::OBSTACLE_SPAWN_RATE;
                    currentMoveSpeed = Constants::Medium::MOVE_SPEED;
                    startGame();
                }
                else if (hardButton.isMouseOver(window)) {
                    hardButton.playClickSound();
                    currentDifficulty = Difficulty::Hard;
                    currentScrollSpeed = Constants::Hard::SCROLL_SPEED;
                    currentFuelConsumption = Constants::Hard::FUEL_CONSUMPTION;
                    currentObstacleSpawnRate = Constants::Hard::OBSTACLE_SPAWN_RATE;
                    currentMoveSpeed = Constants::Hard::MOVE_SPEED;
                    startGame();
                }
            }
        }

        easyButton.setHighlight(easyButton.isMouseOver(window));
        mediumButton.setHighlight(mediumButton.isMouseOver(window));
        hardButton.setHighlight(hardButton.isMouseOver(window));
    }

    void handleOptionsInput() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                currentState = GameState::Menu;
                return;
            }

            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                if (helpButton.isMouseOver(window)) {
                    helpButton.playClickSound();
                    currentState = GameState::Help;
                }
                else if (settingsButton.isMouseOver(window)) {
                    settingsButton.playClickSound();
                    currentState = GameState::Settings;
                }
                else if (backButton.isMouseOver(window)) {
                    backButton.playClickSound();
                    currentState = GameState::Menu;
                }
            }
        }

        helpButton.setHighlight(helpButton.isMouseOver(window));
        settingsButton.setHighlight(settingsButton.isMouseOver(window));
        backButton.setHighlight(backButton.isMouseOver(window));
    }

    void handleHelpInput() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            if ((event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) ||
                (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left &&
                    backButton.isMouseOver(window))) {
                backButton.playClickSound();
                currentState = GameState::Options;
                return;
            }
        }

        backButton.setHighlight(backButton.isMouseOver(window));
    }

    void handleSettingsInput() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            if ((event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) ||
                (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left &&
                    backButton.isMouseOver(window))) {
                backButton.playClickSound();
                currentState = GameState::Options;
                return;
            }
        }

        backButton.setHighlight(backButton.isMouseOver(window));
    }

    void handleCreditsInput() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            if ((event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) ||
                (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left &&
                    backButton.isMouseOver(window))) {
                backButton.playClickSound();
                currentState = GameState::Menu;
                return;
            }
        }

        backButton.setHighlight(backButton.isMouseOver(window));
    }

    void handleHighScoresInput() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            if ((event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) ||
                (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left &&
                    backButton.isMouseOver(window))) {
                backButton.playClickSound();
                currentState = GameState::Menu;
                return;
            }
        }

        backButton.setHighlight(backButton.isMouseOver(window));
    }

    void handleGameInput() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                currentState = GameState::Paused;
                engineSound.pause();
                gameMusic.pause();
                return;
            }

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space && !gameStarted) {
                gameStarted = true;
                if (engineSound.getStatus() != sf::Sound::Playing) {
                    engineSound.play();
                }
            }
        }
    }

    void handlePauseInput() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                currentState = GameState::Playing;
                engineSound.play();
                gameMusic.play();
                return;
            }

            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                if (resumeButton.isMouseOver(window)) {
                    resumeButton.playClickSound();
                    currentState = GameState::Playing;
                    engineSound.play();
                    gameMusic.play();
                }
                else if (pauseQuitButton.isMouseOver(window)) {
                    pauseQuitButton.playClickSound();
                    endGame();
                }
            }
        }

        resumeButton.setHighlight(resumeButton.isMouseOver(window));
        pauseQuitButton.setHighlight(pauseQuitButton.isMouseOver(window));
    }

    void handleGameOverInput() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                if (restartButton.isMouseOver(window)) {
                    restartButton.playClickSound();
                    startGame();
                }
                else if (gameOverBackButton.isMouseOver(window)) {
                    gameOverBackButton.playClickSound();
                    endGame();
                }
            }
        }

        restartButton.setHighlight(restartButton.isMouseOver(window));
        gameOverBackButton.setHighlight(gameOverBackButton.isMouseOver(window));
    }

    void startGame() {
        currentState = GameState::Playing;
        gameStarted = false;
        gameOver = false;
        isLanded = false;
        obstacles.clear();
        coins.clear();
        fuelBottles.clear();
        obstacleSpawnTimer = 0.f;
        score = 0;
        fuel = Constants::MAX_FUEL;
        coin5Clock.restart();
        coin10Clock.restart();
        coin50Clock.restart();
        fuelBottleClock.restart();
        fuelClock.restart();

        helicopter.setPosition(Constants::WINDOW_WIDTH / 4.0f, Constants::WINDOW_HEIGHT / 2.0f);

        for (int i = 0; i < 2; ++i) {
            bgSprites[i].setPosition(i * static_cast<float>(Constants::WINDOW_WIDTH), 0.f);
        }

        bgMusic.stop();
        engineSound.stop();
        crashSound.stop();
        if (gameMusic.getStatus() != sf::Music::Playing) {
            gameMusic.play();
        }
    }

    void endGame() {
        currentState = GameState::Menu;
        gameStarted = false;
        gameOver = false;
        engineSound.stop();
        crashSound.stop();
        gameMusic.stop();
        bgMusic.play();
    }

    void gameOverState() {
        addHighScore(playerName, score, currentDifficulty);
        currentState = GameState::GameOver;
        gameStarted = false;
        engineSound.stop();
        crashSound.play();
    }

    void updateGame(float deltaTime) {
        if (!gameStarted || gameOver) return;

        updateFuel(deltaTime);
        updateCoins(deltaTime);
        updateFuelBottles(deltaTime);

        obstacleSpawnTimer += deltaTime;
        if (obstacleSpawnTimer >= currentObstacleSpawnRate) {
            spawnObstacle();
            obstacleSpawnTimer = 0.f;
        }

        if (!isLanded) {
            for (auto& coin : coins) {
                sf::Vector2f pos = coin.getSprite().getPosition();
                pos.x -= currentScrollSpeed * deltaTime;
                coin.getSprite().setPosition(pos);
            }

            for (auto& bottle : fuelBottles) {
                sf::Vector2f pos = bottle.getSprite().getPosition();
                pos.x -= currentScrollSpeed * deltaTime;
                bottle.getSprite().setPosition(pos);
            }
        }

        sf::Vector2f movement(0.f, 0.f);

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
            movement.y -= currentMoveSpeed;
        }

        movement.y += Constants::GRAVITY;
        helicopter.move(movement * deltaTime);

        sf::Vector2f position = helicopter.getPosition();
        sf::FloatRect bounds = helicopter.getGlobalBounds();

        if (position.x - bounds.width / 2.0f < 0) position.x = bounds.width / 2.0f;
        if (position.x + bounds.width / 2.0f > Constants::WINDOW_WIDTH) position.x = Constants::WINDOW_WIDTH - bounds.width / 2.0f;
        if (position.y - bounds.height / 2.0f < 0) position.y = bounds.height / 2.0f;

        isLanded = (position.y + bounds.height / 2.0f >= Constants::WINDOW_HEIGHT - Constants::LANDING_HEIGHT);
        if (isLanded) position.y = Constants::WINDOW_HEIGHT - Constants::LANDING_HEIGHT - bounds.height / 2.0f;

        helicopter.setPosition(position);

        for (auto& obstacle : obstacles) {
            obstacle.update(deltaTime, isLanded, currentScrollSpeed);

            if (obstacle.isActive() && helicopter.getGlobalBounds().intersects(obstacle.getBounds())) {
                gameOver = true;
                gameOverState();
                return;
            }
        }

        obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(),
            [](const Obstacle& o) { return !o.isActive(); }),
            obstacles.end());

        if (!isLanded) {
            for (auto& bg : bgSprites) bg.move(-currentScrollSpeed * deltaTime, 0.f);

            if (bgSprites[0].getPosition().x + static_cast<float>(Constants::WINDOW_WIDTH) < 0)
                bgSprites[0].setPosition(bgSprites[1].getPosition().x + static_cast<float>(Constants::WINDOW_WIDTH), 0.f);
            if (bgSprites[1].getPosition().x + static_cast<float>(Constants::WINDOW_WIDTH) < 0)
                bgSprites[1].setPosition(bgSprites[0].getPosition().x + static_cast<float>(Constants::WINDOW_WIDTH), 0.f);
        }
    }

    void renderMenu() {
        window.clear();
        window.draw(menuBackground);

        sf::Text title("Helicopter Game", font, 50);
        title.setFillColor(sf::Color::White);
        title.setPosition((window.getSize().x - title.getLocalBounds().width) / 2.0f, 80.f);
        window.draw(title);

        playButton.draw(window);
        optionsButton.draw(window);
        highScoresButton.draw(window);
        creditsButton.draw(window);
        exitButton.draw(window);
        window.display();
    }

    void renderNameInput() {
        window.clear();
        window.draw(menuBackground);

        sf::RectangleShape overlay(sf::Vector2f(window.getSize()));
        overlay.setFillColor(sf::Color(0, 0, 0, 180));
        window.draw(overlay);

        window.draw(namePrompt);
        window.draw(nameInputBox);

        nameInputText.setOrigin(nameInputText.getLocalBounds().width / 2.f,
            nameInputText.getLocalBounds().height / 2.f + 5.f);
        window.draw(nameInputText);

        nameSubmitButton.draw(window);

        if (nameSubmitButton.isMouseOver(window) && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            if (playerName.length() < 3) {
                sf::Text error("Name must be at least 3 characters!", font, 24);
                error.setFillColor(sf::Color::Red);
                error.setPosition((window.getSize().x - error.getLocalBounds().width) / 2.0f, 380.f);
                window.draw(error);
            }
        }

        window.display();
    }

    void renderDifficultySelect() {
        window.clear();
        window.draw(menuBackground);

        sf::RectangleShape overlay(sf::Vector2f(window.getSize()));
        overlay.setFillColor(sf::Color(0, 0, 0, 180));
        window.draw(overlay);

        sf::Text title("Select Difficulty", font, 40);
        title.setFillColor(sf::Color::White);
        title.setPosition((window.getSize().x - title.getLocalBounds().width) / 2.0f, 120.f);
        window.draw(title);

        easyButton.draw(window);
        mediumButton.draw(window);
        hardButton.draw(window);

        window.display();
    }

    void renderOptions() {
        window.clear();
        window.draw(menuBackground);

        sf::RectangleShape overlay(sf::Vector2f(window.getSize()));
        overlay.setFillColor(sf::Color(0, 0, 0, 180));
        window.draw(overlay);

        sf::Text title("Options Menu", font, 40);
        title.setFillColor(sf::Color::White);
        title.setPosition((window.getSize().x - title.getLocalBounds().width) / 2.0f, 150.f);
        window.draw(title);

        helpButton.draw(window);
        settingsButton.draw(window);
        backButton.draw(window);
        window.display();
    }

    void renderHelp() {
        window.clear();
        window.draw(menuBackground);

        sf::RectangleShape overlay(sf::Vector2f(window.getSize()));
        overlay.setFillColor(sf::Color(0, 0, 0, 180));
        window.draw(overlay);

        sf::Text title("Help", font, 40);
        title.setFillColor(sf::Color::White);
        title.setPosition((window.getSize().x - title.getLocalBounds().width) / 2.0f, 100.f);
        window.draw(title);

        sf::Text helpText(
            "Game Instructions:\n\n"
            "1. Use Arrow Up key to lift the Helicopter\n\n"
            "2. Avoid obstacles like birds and trees\n\n"
            "3. Collect coins for points (5, 10, 50)\n\n"
            "4. Collect fuel bottles to refill your tank\n\n"
            "5. Watch your fuel - land to regenerate",
            font, 15);
        helpText.setFillColor(sf::Color::White);
        helpText.setPosition(50.f, 180.f);
        window.draw(helpText);

        backButton.draw(window);
        window.display();
    }

    void renderSettings() {
        window.clear();
        window.draw(menuBackground);

        sf::RectangleShape overlay(sf::Vector2f(window.getSize()));
        overlay.setFillColor(sf::Color(0, 0, 0, 180));
        window.draw(overlay);

        sf::Text title("Settings", font, 40);
        title.setFillColor(sf::Color::White);
        title.setPosition((window.getSize().x - title.getLocalBounds().width) / 2.0f, 100.f);
        window.draw(title);

        sf::Text settingsText(
            "Game Settings:\n\n"
            "1. Sound Volume: Adjust sound effects volume\n\n"
            "2. Music Volume: Control background music level\n\n"
            "3. Controls: Change key bindings\n\n"
            "4. Graphics: Adjust quality and resolution\n\n"
            "5. Difficulty: Set game challenge level",
            font, 15);
        settingsText.setFillColor(sf::Color::White);
        settingsText.setPosition(50.f, 180.f);
        window.draw(settingsText);

        backButton.draw(window);
        window.display();
    }

    void renderCredits() {
        window.clear();
        window.draw(menuBackground);

        sf::RectangleShape overlay(sf::Vector2f(window.getSize()));
        overlay.setFillColor(sf::Color(0, 0, 0, 180));
        window.draw(overlay);

        sf::Text title("Credits", font, 40);
        title.setFillColor(sf::Color::White);
        title.setPosition((window.getSize().x - title.getLocalBounds().width) / 2.0f, 100.f);
        window.draw(title);

        sf::Text creditsText("\nDev Kumar       24K-0028\nMasoom Khan   24K-0001", font, 28);
        creditsText.setFillColor(sf::Color::White);
        creditsText.setPosition((window.getSize().x - creditsText.getLocalBounds().width) / 2.0f, 200.f);
        window.draw(creditsText);

        backButton.draw(window);
        window.display();
    }

    void renderHighScores() {
        window.clear();
        window.draw(menuBackground);

        sf::RectangleShape overlay(sf::Vector2f(window.getSize()));
        overlay.setFillColor(sf::Color(0, 0, 0, 180));
        window.draw(overlay);

        sf::Text title("High Scores", font, 40);
        title.setFillColor(sf::Color::White);
        title.setPosition((window.getSize().x - title.getLocalBounds().width) / 2.0f, 80.f);
        window.draw(title);

        // Column headers
        sf::Text rankHeader("Rank", font, 24);
        rankHeader.setFillColor(sf::Color::Yellow);
        rankHeader.setPosition(100.f, 150.f);
        window.draw(rankHeader);

        sf::Text nameHeader("Name", font, 24);
        nameHeader.setFillColor(sf::Color::Yellow);
        nameHeader.setPosition(220.f, 150.f);
        window.draw(nameHeader);

        sf::Text scoreHeader("Score", font, 24);
        scoreHeader.setFillColor(sf::Color::Yellow);
        scoreHeader.setPosition(390.f, 150.f);
        window.draw(scoreHeader);

        sf::Text diffHeader("Difficulty", font, 24);
        diffHeader.setFillColor(sf::Color::Yellow);
        diffHeader.setPosition(530.f, 150.f);
        window.draw(diffHeader);

        // Display top 7 scores
        int entriesToShow = std::min(7, static_cast<int>(highScores.size()));
        for (int i = 0; i < entriesToShow; ++i) {
            const auto& entry = highScores[i];
            std::string difficultyStr;
            switch (entry.difficulty) {
            case Difficulty::Easy: difficultyStr = "Easy"; break;
            case Difficulty::Medium: difficultyStr = "Medium"; break;
            case Difficulty::Hard: difficultyStr = "Hard"; break;
            }

            // Rank column (right aligned)
            sf::Text rankText(std::to_string(i + 1) + ".", font, 20);
            rankText.setFillColor(sf::Color::White);
            rankText.setPosition(110.f, 190.f + i * 30.f);
            window.draw(rankText);

            // Name column (left aligned)
            sf::Text nameText(entry.name, font, 20);
            nameText.setFillColor(sf::Color::White);
            nameText.setPosition(220.f, 190.f + i * 30.f);
            window.draw(nameText);

            // Score column (right aligned)
            sf::Text scoreText(std::to_string(entry.score), font, 20);
            scoreText.setFillColor(sf::Color::White);
            scoreText.setPosition(390.f, 190.f + i * 30.f);
            window.draw(scoreText);

            // Difficulty column (left aligned)
            sf::Text diffText(difficultyStr, font, 20);
            diffText.setFillColor(sf::Color::White);
            diffText.setPosition(530.f, 190.f + i * 30.f);
            window.draw(diffText);
        }

        backButton.draw(window);
        window.display();
    }

    void renderGame() {
        window.clear();

        if (gameStarted) {
            for (const auto& bg : bgSprites) window.draw(bg);
            for (const auto& coin : coins) coin.draw(window);
            for (const auto& bottle : fuelBottles) bottle.draw(window);
            for (const auto& obstacle : obstacles) obstacle.draw(window);
            window.draw(helicopter);

            sf::Text playerText("Player: " + playerName, font, 20);
            playerText.setFillColor(sf::Color::White);
            playerText.setPosition(20.f, 20.f);
            window.draw(playerText);

            sf::Text scoreText("Score: " + std::to_string(score), font, 20);
            scoreText.setFillColor(sf::Color::White);
            scoreText.setPosition(20.f, 50.f);
            window.draw(scoreText);

            window.draw(fuelBackground);
            window.draw(fuelBar);
            window.draw(fuelText);
        }
        else {
            window.draw(bgSprites[0]);
            sf::RectangleShape overlay(sf::Vector2f(Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT));
            overlay.setFillColor(sf::Color(0, 0, 0, 150));
            window.draw(overlay);

            sf::Text startText("Press SPACE to Start", font, 24);
            startText.setFillColor(sf::Color::White);
            startText.setPosition(Constants::WINDOW_WIDTH / 2.0f - startText.getGlobalBounds().width / 2.0f,
                Constants::WINDOW_HEIGHT / 2.0f - startText.getGlobalBounds().height / 2.0f);
            window.draw(startText);

            sf::Text playerText("Player: " + playerName, font, 20);
            playerText.setFillColor(sf::Color::White);
            playerText.setPosition(20.f, 20.f);
            window.draw(playerText);

            window.draw(fuelBackground);
            window.draw(fuelBar);
            window.draw(fuelText);
        }

        window.display();
    }

    void renderPause() {
        window.clear();

        for (const auto& bg : bgSprites) window.draw(bg);
        for (const auto& coin : coins) coin.draw(window);
        for (const auto& bottle : fuelBottles) bottle.draw(window);
        for (const auto& obstacle : obstacles) obstacle.draw(window);
        window.draw(helicopter);

        sf::RectangleShape overlay(sf::Vector2f(Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT));
        overlay.setFillColor(sf::Color(0, 0, 0, 180));
        window.draw(overlay);

        sf::Text pauseText("PAUSED", font, 60);
        pauseText.setFillColor(sf::Color::White);
        sf::FloatRect bounds = pauseText.getLocalBounds();
        pauseText.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
        pauseText.setPosition(Constants::WINDOW_WIDTH / 2.0f, 150.f);
        window.draw(pauseText);

        resumeButton.draw(window);
        pauseQuitButton.draw(window);

        window.display();
    }

    void renderGameOver() {
        window.clear();

        for (const auto& bg : bgSprites) window.draw(bg);
        for (const auto& coin : coins) coin.draw(window);
        for (const auto& bottle : fuelBottles) bottle.draw(window);
        for (const auto& obstacle : obstacles) obstacle.draw(window);
        window.draw(helicopter);

        sf::RectangleShape overlay(sf::Vector2f(Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT));
        overlay.setFillColor(sf::Color(0, 0, 0, 200));
        window.draw(overlay);

        sf::Text gameOverText("GAME OVER", font, 60);
        gameOverText.setFillColor(sf::Color::Red);
        gameOverText.setStyle(sf::Text::Bold);
        sf::FloatRect bounds = gameOverText.getLocalBounds();
        gameOverText.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
        gameOverText.setPosition(Constants::WINDOW_WIDTH / 2.0f, 145.f);
        window.draw(gameOverText);

        sf::Text playerText("Player: " + playerName, font, 30);
        playerText.setFillColor(sf::Color::White);
        playerText.setPosition((Constants::WINDOW_WIDTH - playerText.getLocalBounds().width) / 2.0f, 195.f);
        window.draw(playerText);

        sf::Text scoreText("Score: " + std::to_string(score), font, 30);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setPosition((Constants::WINDOW_WIDTH - scoreText.getLocalBounds().width) / 2.0f, 245.f);
        window.draw(scoreText);

        restartButton.draw(window);
        gameOverBackButton.draw(window);

        window.display();
    }

public:
    HelicopterGame() : window(sf::VideoMode(Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT), "Helicopter Game", sf::Style::Default),
        currentState(GameState::Menu),
        currentDifficulty(Difficulty::Medium),
        nameSubmitButton("", font, 0, sf::Color::White, sf::Color::White, sf::Vector2f(0, 0), sf::Vector2f(0, 0)),
        playButton("", font, 0, sf::Color::White, sf::Color::White, sf::Vector2f(0, 0), sf::Vector2f(0, 0)),
        optionsButton("", font, 0, sf::Color::White, sf::Color::White, sf::Vector2f(0, 0), sf::Vector2f(0, 0)),
        creditsButton("", font, 0, sf::Color::White, sf::Color::White, sf::Vector2f(0, 0), sf::Vector2f(0, 0)),
        exitButton("", font, 0, sf::Color::White, sf::Color::White, sf::Vector2f(0, 0), sf::Vector2f(0, 0)),
        helpButton("", font, 0, sf::Color::White, sf::Color::White, sf::Vector2f(0, 0), sf::Vector2f(0, 0)),
        settingsButton("", font, 0, sf::Color::White, sf::Color::White, sf::Vector2f(0, 0), sf::Vector2f(0, 0)),
        backButton("", font, 0, sf::Color::White, sf::Color::White, sf::Vector2f(0, 0), sf::Vector2f(0, 0)),
        restartButton("", font, 0, sf::Color::White, sf::Color::White, sf::Vector2f(0, 0), sf::Vector2f(0, 0)),
        gameOverBackButton("", font, 0, sf::Color::White, sf::Color::White, sf::Vector2f(0, 0), sf::Vector2f(0, 0)),
        resumeButton("", font, 0, sf::Color::White, sf::Color::White, sf::Vector2f(0, 0), sf::Vector2f(0, 0)),
        pauseQuitButton("", font, 0, sf::Color::White, sf::Color::White, sf::Vector2f(0, 0), sf::Vector2f(0, 0)),
        easyButton("", font, 0, sf::Color::White, sf::Color::White, sf::Vector2f(0, 0), sf::Vector2f(0, 0)),
        mediumButton("", font, 0, sf::Color::White, sf::Color::White, sf::Vector2f(0, 0), sf::Vector2f(0, 0)),
        hardButton("", font, 0, sf::Color::White, sf::Color::White, sf::Vector2f(0, 0), sf::Vector2f(0, 0)),
        highScoresButton("", font, 0, sf::Color::White, sf::Color::White, sf::Vector2f(0, 0), sf::Vector2f(0, 0)) {
        window.setFramerateLimit(60);
        loadResources();
    }

    void run() {
        while (window.isOpen()) {
            float deltaTime = gameClock.restart().asSeconds();

            switch (currentState) {
            case GameState::Menu:
                handleMenuInput();
                renderMenu();
                break;
            case GameState::NameInput:
                handleNameInput();
                renderNameInput();
                break;
            case GameState::DifficultySelect:
                handleDifficultyInput();
                renderDifficultySelect();
                break;
            case GameState::Playing:
                handleGameInput();
                updateGame(deltaTime);
                renderGame();
                break;
            case GameState::GameOver:
                handleGameOverInput();
                renderGameOver();
                break;
            case GameState::Options:
                handleOptionsInput();
                renderOptions();
                break;
            case GameState::Help:
                handleHelpInput();
                renderHelp();
                break;
            case GameState::Settings:
                handleSettingsInput();
                renderSettings();
                break;
            case GameState::Credits:
                handleCreditsInput();
                renderCredits();
                break;
            case GameState::Paused:
                handlePauseInput();
                renderPause();
                break;
            case GameState::HighScores:
                handleHighScoresInput();
                renderHighScores();
                break;
            }
        }
    }
};

int main() {
    try {
        HelicopterGame game;
        game.run();
    }
    catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}