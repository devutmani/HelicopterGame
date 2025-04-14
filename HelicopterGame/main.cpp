#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <random>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <memory>

using namespace std;
using namespace sf;

// Game Constants
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
float GRAVITY = 0.3f;
float FLAP_FORCE = -7.0f;
float OBSTACLE_SPEED = 3.0f;
int OBSTACLE_FREQUENCY = 150;
const int MAX_OBSTACLES = 20;
const int MAX_HIGH_SCORES = 10;
const float BG_SCROLL_SPEED = 0.5f;
const int MAX_PARTICLES = 500;

// Game States
enum GameState {
    MAIN_MENU,
    USERNAME_INPUT,
    LEVEL_SELECT,
    PLAYING,
    PAUSED,
    GAME_OVER,
    HIGH_SCORES,
    TUTORIAL,
    SETTINGS
};

// Difficulty Levels
enum Difficulty {
    EASY,
    MEDIUM,
    HARD
};

Difficulty currentDifficulty = MEDIUM;

// Obstacle types
enum ObstacleType {
    ROCK,
    BIRD,
    TREE,
    CLOUD,
    POWERUP,
    NONE
};

// Power-up types
enum PowerUpType {
    SHIELD,
    SPEED_BOOST,
    SCORE_MULTIPLIER,
    INVINCIBILITY
};

// High score structure
struct HighScore {
    string name;
    int score = 0;
    Difficulty difficulty = MEDIUM;
};

// Game settings
struct GameSettings {
    bool soundEnabled = true;
    bool musicEnabled = true;
    float soundVolume = 100.f;
    float musicVolume = 50.f;
};

HighScore highScores[MAX_HIGH_SCORES];
int numHighScores = 0;
string currentPlayerName = "Player";
GameSettings gameSettings;

// Forward declarations
static void setDifficulty(Difficulty difficulty);
static void loadHighScores();
static void saveHighScores();
static void addHighScore(int score);
static void loadSettings();
static void saveSettings();

class Button {
private:
    RectangleShape shape;
    Text text;
    Color idleColor;
    Color hoverColor;
    Color activeColor;

public:
    Button(float x, float y, float width, float height,
        Font& font, string btnText,
        Color idle, Color hover, Color active) {
        shape.setPosition(Vector2f(x, y));
        shape.setSize(Vector2f(width, height));

        text.setFont(font);
        text.setString(btnText);
        text.setFillColor(Color::White);
        text.setCharacterSize(24);
        text.setPosition(
            x + (width / 2.0f) - text.getLocalBounds().width / 2.0f,
            y + (height / 2.0f) - text.getLocalBounds().height / 2.0f
        );

        idleColor = idle;
        hoverColor = hover;
        activeColor = active;

        shape.setFillColor(idleColor);
    }

    void draw(RenderWindow& window) const {
        window.draw(shape);
        window.draw(text);
    }

    bool isMouseOver(RenderWindow& window) const {
        Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window));
        return shape.getGlobalBounds().contains(mousePos);
    }

    void update(RenderWindow& window) {
        if (isMouseOver(window)) {
            shape.setFillColor(hoverColor);
            if (Mouse::isButtonPressed(Mouse::Left)) {
                shape.setFillColor(activeColor);
            }
        }
        else {
            shape.setFillColor(idleColor);
        }
    }
};

struct Particle {
    Vector2f position;
    Vector2f velocity;
    Color color;
    float lifetime;
};

class ParticleSystem {
private:
    unique_ptr<Particle[]> particles;
    int activeParticles;

public:
    ParticleSystem() : particles(make_unique<Particle[]>(MAX_PARTICLES)), activeParticles(0) {}

    void createExplosion(Vector2f position, int count = 50) {
        for (int i = 0; i < count && activeParticles < MAX_PARTICLES; i++) {
            particles[activeParticles].position = position;
            particles[activeParticles].velocity = Vector2f(
                static_cast<float>(rand() % 100 - 50) / 10.0f,
                static_cast<float>(rand() % 100 - 50) / 10.0f
            );
            particles[activeParticles].color = Color(255, rand() % 255, 0);
            particles[activeParticles].lifetime = static_cast<float>(rand() % 100) / 100.0f * 2.0f;
            activeParticles++;
        }
    }

    void update(float dt) {
        for (int i = 0; i < activeParticles; i++) {
            particles[i].position += particles[i].velocity * dt * 60.0f;
            particles[i].lifetime -= dt;
            particles[i].color.a = static_cast<Uint8>(255.0f * (particles[i].lifetime / 2.0f));
        }

        // Remove dead particles
        int newActive = 0;
        for (int i = 0; i < activeParticles; i++) {
            if (particles[i].lifetime > 0.0f) {
                particles[newActive++] = particles[i];
            }
        }
        activeParticles = newActive;
    }

    void draw(RenderWindow& window) const {
        CircleShape shape(3.0f);
        for (int i = 0; i < activeParticles; i++) {
            shape.setPosition(particles[i].position);
            shape.setFillColor(particles[i].color);
            window.draw(shape);
        }
    }
};

class Animation {
private:
    IntRect frames[10];
    int frameCount;
    float frameTime;
    float currentTime;
    int currentFrame;
    bool looping;

public:
    Animation() : frameCount(0), frameTime(0.1f), currentTime(0.0f), currentFrame(0), looping(true) {}

    void addFrame(IntRect frame) {
        if (frameCount < 10) {
            frames[frameCount++] = frame;
        }
    }

    void update(float dt) {
        if (frameCount == 0) return;

        currentTime += dt;
        if (currentTime >= frameTime) {
            currentTime = 0.0f;
            currentFrame++;
            if (currentFrame >= frameCount) {
                if (looping) currentFrame = 0;
                else currentFrame = frameCount - 1;
            }
        }
    }

    IntRect getCurrentFrame() const {
        if (frameCount == 0) return IntRect();
        return frames[currentFrame];
    }

    void setFrameTime(float time) { frameTime = time; }
    void setLooping(bool loop) { looping = loop; }
};

class Obstacle {
public:
    ObstacleType type = NONE;
    PowerUpType powerType = SHIELD;
    bool active = false;
    Sprite sprite;
    Animation animation;
    float yPos = 0.0f;

    void create(ObstacleType t, float x, float y, Texture& texture) {
        type = t;
        active = true;
        yPos = y;

        switch (type) {
        case ROCK:
            sprite.setTexture(texture);
            sprite.setTextureRect(IntRect(0, 0, 60, 60));
            sprite.setPosition(x, static_cast<float>(WINDOW_HEIGHT - 60));
            break;
        case BIRD:
            sprite.setTexture(texture);
            for (int i = 0; i < 4; i++) {
                animation.addFrame(IntRect(i * 40, 0, 40, 30));
            }
            animation.setFrameTime(0.15f);
            sprite.setPosition(x, y);
            break;
        case TREE:
            sprite.setTexture(texture);
            sprite.setTextureRect(IntRect(0, 0, 20, 60));
            sprite.setPosition(x, static_cast<float>(WINDOW_HEIGHT - 60 - 20));
            break;
        case CLOUD:
            sprite.setTexture(texture);
            sprite.setTextureRect(IntRect(0, 0, 80, 40));
            sprite.setPosition(x, y);
            break;
        case POWERUP:
            sprite.setTexture(texture);
            powerType = static_cast<PowerUpType>(rand() % 4);
            sprite.setTextureRect(IntRect(powerType * 30, 0, 30, 30));
            sprite.setPosition(x, y);
            for (int i = 0; i < 4; i++) {
                animation.addFrame(IntRect(powerType * 30 + i * 30, 0, 30, 30));
            }
            animation.setFrameTime(0.2f);
            break;
        case NONE:
            break;
        }
    }

    void update(float dt) {
        if (!active) return;

        float speed = OBSTACLE_SPEED;
        if (type == BIRD) speed *= 1.2f;
        if (type == CLOUD) speed *= 0.7f;
        if (type == POWERUP) speed *= 0.8f;

        sprite.move(-speed, 0.0f);

        if (type == BIRD || type == POWERUP) {
            animation.update(dt);
            sprite.setTextureRect(animation.getCurrentFrame());
        }
    }

    void draw(RenderWindow& window) const {
        if (!active) return;
        window.draw(sprite);
    }

    bool isOffScreen() const {
        return active && (sprite.getPosition().x + sprite.getLocalBounds().width < 0.0f);
    }

    FloatRect getBounds() const {
        return sprite.getGlobalBounds();
    }
};

class Helicopter {
private:
    Sprite sprite;
    Animation animation;
    Vector2f velocity;
    float rotationAngle;
    bool hasShield;
    bool isInvincible;
    float powerUpTimer;
    int scoreMultiplier;

public:
    Helicopter(Texture& texture) :
        hasShield(false),
        isInvincible(false),
        powerUpTimer(0.0f),
        scoreMultiplier(1) {
        sprite.setTexture(texture);
        for (int i = 0; i < 4; i++) {
            animation.addFrame(IntRect(i * 50, 0, 50, 20));
        }
        animation.setFrameTime(0.1f);
        sprite.setPosition(100.0f, static_cast<float>(WINDOW_HEIGHT) / 2.0f);
        sprite.setOrigin(25.0f, 10.0f);

        velocity = Vector2f(0.0f, 0.0f);
        rotationAngle = 0.0f;
    }

    void flap() {
        velocity.y = FLAP_FORCE;
        rotationAngle = -20.0f;
    }

    void update(float dt) {
        // Apply gravity
        velocity.y += GRAVITY;

        // Update position
        sprite.move(0.0f, velocity.y);

        // Update animation
        animation.update(dt);
        sprite.setTextureRect(animation.getCurrentFrame());

        // Rotate based on velocity
        if (rotationAngle < 0.0f) rotationAngle += 1.0f;
        else if (velocity.y > 0.0f) rotationAngle = min(rotationAngle + 0.5f, 20.0f);

        sprite.setRotation(rotationAngle);

        // Update power-up timers
        if (powerUpTimer > 0.0f) {
            powerUpTimer -= dt;
            if (powerUpTimer <= 0.0f) {
                resetPowerUps();
            }
        }

        // Keep helicopter within screen bounds
        Vector2f position = sprite.getPosition();
        if (position.y < 0.0f) {
            position.y = 0.0f;
            velocity.y = 0.0f;
        }
        else if (position.y > WINDOW_HEIGHT - 50.0f) {
            position.y = static_cast<float>(WINDOW_HEIGHT - 50);
            velocity.y = 0.0f;
        }
        sprite.setPosition(position);
    }

    void draw(RenderWindow& window) const {
        window.draw(sprite);
        if (hasShield) {
            CircleShape shield(30.0f);
            shield.setFillColor(Color(0, 0, 255, 100));
            shield.setOutlineColor(Color::Blue);
            shield.setOutlineThickness(2.0f);
            shield.setOrigin(30.0f, 30.0f);
            shield.setPosition(sprite.getPosition());
            window.draw(shield);
        }
    }

    void applyPowerUp(PowerUpType type, float duration = 10.0f) {
        switch (type) {
        case SHIELD:
            hasShield = true;
            break;
        case SPEED_BOOST:
            FLAP_FORCE = -9.0f;
            break;
        case SCORE_MULTIPLIER:
            scoreMultiplier = 2;
            break;
        case INVINCIBILITY:
            isInvincible = true;
            break;
        }
        powerUpTimer = duration;
    }

    void resetPowerUps() {
        hasShield = false;
        isInvincible = false;
        scoreMultiplier = 1;
        setDifficulty(currentDifficulty);
    }

    bool checkCollision(const FloatRect& bounds) const {
        if (isInvincible) return false;
        if (hasShield) {
            FloatRect shieldBounds(
                sprite.getPosition().x - 30.0f,
                sprite.getPosition().y - 30.0f,
                60.0f, 60.0f
            );
            return shieldBounds.intersects(bounds);
        }
        return sprite.getGlobalBounds().intersects(bounds);
    }

    int getScoreMultiplier() const { return scoreMultiplier; }

    FloatRect getBounds() const {
        return sprite.getGlobalBounds();
    }
};

class Background {
private:
    Texture texture;
    Sprite sprite1;
    Sprite sprite2;
    float offset1;
    float offset2;

public:
    Background(const string& filename) : offset1(0.0f), offset2(static_cast<float>(WINDOW_WIDTH)) {
        if (!texture.loadFromFile(filename)) {
            cerr << "Failed to load background texture" << endl;
        }
        sprite1.setTexture(texture);
        sprite2.setTexture(texture);
        sprite1.setPosition(0.0f, 0.0f);
        sprite2.setPosition(static_cast<float>(WINDOW_WIDTH), 0.0f);
    }

    void update(float dt) {
        offset1 -= BG_SCROLL_SPEED * dt * 60.0f;
        offset2 -= BG_SCROLL_SPEED * dt * 60.0f;

        if (offset1 <= -WINDOW_WIDTH) offset1 = static_cast<float>(WINDOW_WIDTH);
        if (offset2 <= -WINDOW_WIDTH) offset2 = static_cast<float>(WINDOW_WIDTH);

        sprite1.setPosition(offset1, 0.0f);
        sprite2.setPosition(offset2, 0.0f);
    }

    void draw(RenderWindow& window) const {
        window.draw(sprite1);
        window.draw(sprite2);
    }
};

static void loadHighScores() {
    ifstream file("highscores.txt");
    if (file.is_open()) {
        numHighScores = 0;
        while (numHighScores < MAX_HIGH_SCORES) {
            int difficultyValue;
            file >> highScores[numHighScores].name
                >> highScores[numHighScores].score
                >> difficultyValue;

            if (file.fail()) break;

            highScores[numHighScores].difficulty = static_cast<Difficulty>(difficultyValue);
            numHighScores++;
        }
        file.close();
    }
}

static void saveHighScores() {
    ofstream file("highscores.txt");
    if (file.is_open()) {
        for (int i = 0; i < numHighScores; i++) {
            file << highScores[i].name << " "
                << highScores[i].score << " "
                << static_cast<int>(highScores[i].difficulty) << "\n";
        }
        file.close();
    }
}

static void addHighScore(int score) {
    if (numHighScores < MAX_HIGH_SCORES) {
        highScores[numHighScores].name = currentPlayerName;
        highScores[numHighScores].score = score;
        highScores[numHighScores].difficulty = currentDifficulty;
        numHighScores++;
    }
    else {
        int minIndex = 0;
        for (int i = 1; i < MAX_HIGH_SCORES; i++) {
            if (highScores[i].score < highScores[minIndex].score) {
                minIndex = i;
            }
        }

        if (score > highScores[minIndex].score) {
            highScores[minIndex].name = currentPlayerName;
            highScores[minIndex].score = score;
            highScores[minIndex].difficulty = currentDifficulty;
        }
    }

    sort(highScores, highScores + numHighScores, [](const HighScore& a, const HighScore& b) {
        return a.score > b.score;
        });

    saveHighScores();
}

static void loadSettings() {
    ifstream file("settings.cfg");
    if (file.is_open()) {
        file >> gameSettings.soundEnabled
            >> gameSettings.musicEnabled
            >> gameSettings.soundVolume
            >> gameSettings.musicVolume;
        file.close();
    }
}

static void saveSettings() {
    ofstream file("settings.cfg");
    if (file.is_open()) {
        file << gameSettings.soundEnabled << "\n"
            << gameSettings.musicEnabled << "\n"
            << gameSettings.soundVolume << "\n"
            << gameSettings.musicVolume;
        file.close();
    }
}

static void setDifficulty(Difficulty difficulty) {
    currentDifficulty = difficulty;
    switch (difficulty) {
    case EASY:
        GRAVITY = 0.2f;
        FLAP_FORCE = -8.0f;
        OBSTACLE_SPEED = 2.5f;
        OBSTACLE_FREQUENCY = 200;
        break;
    case MEDIUM:
        GRAVITY = 0.3f;
        FLAP_FORCE = -7.0f;
        OBSTACLE_SPEED = 3.0f;
        OBSTACLE_FREQUENCY = 150;
        break;
    case HARD:
        GRAVITY = 0.4f;
        FLAP_FORCE = -6.0f;
        OBSTACLE_SPEED = 3.5f;
        OBSTACLE_FREQUENCY = 100;
        break;
    }
}

int main() {
    RenderWindow window(VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Helicopter Game");
    window.setFramerateLimit(60);

    // Load resources
    Font font;
    if (!font.loadFromFile("arial.ttf")) {
        cerr << "Failed to load font. Using default font." << endl;
    }

    Texture helicopterTexture;
    if (!helicopterTexture.loadFromFile("helicopter.png")) {
        cerr << "Failed to load helicopter texture" << endl;
    }

    Texture obstacleTexture;
    if (!obstacleTexture.loadFromFile("obstacles.png")) {
        cerr << "Failed to load obstacles texture" << endl;
    }

    Texture powerupTexture;
    if (!powerupTexture.loadFromFile("powerups.png")) {
        cerr << "Failed to load powerups texture" << endl;
    }

    // Load sounds
    SoundBuffer flapSoundBuffer;
    SoundBuffer crashSoundBuffer;
    SoundBuffer powerupSoundBuffer;
    Music backgroundMusic;

    if (!flapSoundBuffer.loadFromFile("flap.wav")) {
        cerr << "Failed to load flap sound" << endl;
    }
    Sound flapSound(flapSoundBuffer);

    if (!crashSoundBuffer.loadFromFile("crash.wav")) {
        cerr << "Failed to load crash sound" << endl;
    }
    Sound crashSound(crashSoundBuffer);

    if (!powerupSoundBuffer.loadFromFile("powerup.wav")) {
        cerr << "Failed to load powerup sound" << endl;
    }
    Sound powerupSound(powerupSoundBuffer);

    if (!backgroundMusic.openFromFile("background.ogg")) {
        cerr << "Failed to load background music" << endl;
    }
    backgroundMusic.setLoop(true);

    // Load game data
    loadHighScores();
    loadSettings();

    // Set volumes
    flapSound.setVolume(gameSettings.soundVolume);
    crashSound.setVolume(gameSettings.soundVolume);
    powerupSound.setVolume(gameSettings.soundVolume);
    backgroundMusic.setVolume(gameSettings.musicVolume);

    if (gameSettings.musicEnabled) {
        backgroundMusic.play();
    }

    // Game objects
    Helicopter helicopter(helicopterTexture);
    unique_ptr<Obstacle[]> obstacles = make_unique<Obstacle[]>(MAX_OBSTACLES);
    int activeObstacles = 0;
    ParticleSystem particles;
    Background background("background.png");

    // Random number generation
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> heightDist(50, WINDOW_HEIGHT - 150);
    uniform_int_distribution<> cloudHeightDist(50, 200);
    uniform_int_distribution<> obstacleTypeDist(0, 4);
    uniform_int_distribution<> powerupDist(0, 100);

    // Game state
    GameState gameState = MAIN_MENU;
    Clock gameClock;
    float deltaTime = 0.0f;
    int obstacleTimer = 0;
    int score = 0;
    bool gameOver = false;
    String playerNameInput;
    bool nameEntered = false;

    // UI Elements
    Text titleText;
    titleText.setFont(font);
    titleText.setString("Helicopter Game");
    titleText.setCharacterSize(48);
    titleText.setFillColor(Color::White);
    titleText.setPosition(WINDOW_WIDTH / 2.0f - titleText.getLocalBounds().width / 2.0f, 50.0f);

    Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(Color::White);
    scoreText.setPosition(10.0f, 10.0f);

    Text pauseText;
    pauseText.setFont(font);
    pauseText.setString("PAUSED");
    pauseText.setCharacterSize(48);
    pauseText.setFillColor(Color::White);
    pauseText.setPosition(WINDOW_WIDTH / 2.0f - pauseText.getLocalBounds().width / 2.0f, 150.0f);

    // Username input elements
    Text namePromptText;
    namePromptText.setFont(font);
    namePromptText.setString("Enter your name:");
    namePromptText.setCharacterSize(30);
    namePromptText.setFillColor(Color::White);
    namePromptText.setPosition(WINDOW_WIDTH / 2.0f - namePromptText.getLocalBounds().width / 2.0f, 200.0f);

    Text nameInputText;
    nameInputText.setFont(font);
    nameInputText.setString("");
    nameInputText.setCharacterSize(30);
    nameInputText.setFillColor(Color::White);
    nameInputText.setPosition(WINDOW_WIDTH / 2.0f - 100.0f, 250.0f);

    // Buttons
    Button playButton(WINDOW_WIDTH / 2.0f - 100.0f, 200.0f, 200.0f, 50.0f, font, "Play Game",
        Color::Blue, Color::Cyan, Color::Green);
    Button scoresButton(WINDOW_WIDTH / 2.0f - 100.0f, 270.0f, 200.0f, 50.0f, font, "High Scores",
        Color::Blue, Color::Cyan, Color::Green);
    Button settingsButton(WINDOW_WIDTH / 2.0f - 100.0f, 340.0f, 200.0f, 50.0f, font, "Settings",
        Color::Blue, Color::Cyan, Color::Green);
    Button tutorialButton(WINDOW_WIDTH / 2.0f - 100.0f, 410.0f, 200.0f, 50.0f, font, "How to Play",
        Color::Blue, Color::Cyan, Color::Green);
    Button exitButton(WINDOW_WIDTH / 2.0f - 100.0f, 480.0f, 200.0f, 50.0f, font, "Exit Game",
        Color::Blue, Color::Cyan, Color::Green);
    Button resumeButton(WINDOW_WIDTH / 2.0f - 100.0f, 200.0f, 200.0f, 50.0f, font, "Resume",
        Color::Blue, Color::Cyan, Color::Green);
    Button easyButton(WINDOW_WIDTH / 2.0f - 100.0f, 200.0f, 200.0f, 50.0f, font, "Easy",
        Color::Blue, Color::Cyan, Color::Green);
    Button mediumButton(WINDOW_WIDTH / 2.0f - 100.0f, 270.0f, 200.0f, 50.0f, font, "Medium",
        Color::Blue, Color::Cyan, Color::Green);
    Button hardButton(WINDOW_WIDTH / 2.0f - 100.0f, 340.0f, 200.0f, 50.0f, font, "Hard",
        Color::Blue, Color::Cyan, Color::Green);
    Button backButton(WINDOW_WIDTH / 2.0f - 100.0f, 410.0f, 200.0f, 50.0f, font, "Back",
        Color::Blue, Color::Cyan, Color::Green);
    Button continueButton(WINDOW_WIDTH / 2.0f - 100.0f, 320.0f, 200.0f, 50.0f, font, "Continue",
        Color::Blue, Color::Cyan, Color::Green);

    // Create initial clouds
    for (int i = 0; i < 5 && activeObstacles < MAX_OBSTACLES; i++) {
        obstacles[activeObstacles++].create(CLOUD, static_cast<float>(i * 200),
            static_cast<float>(cloudHeightDist(gen)), obstacleTexture);
    }

    // Game loop
    while (window.isOpen()) {
        deltaTime = gameClock.restart().asSeconds();

        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
            }

            // Handle text input for username
            if (gameState == USERNAME_INPUT) {
                if (event.type == Event::TextEntered) {
                    if (event.text.unicode == '\b') { // Backspace
                        if (!playerNameInput.isEmpty()) {
                            playerNameInput.erase(playerNameInput.getSize() - 1);
                        }
                    }
                    else if (event.text.unicode < 128 && event.text.unicode != '\r') {
                        if (playerNameInput.getSize() < 15) {
                            playerNameInput += static_cast<char>(event.text.unicode);
                        }
                    }
                    nameInputText.setString(playerNameInput);
                }
            }

            if (event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::Escape) {
                    if (gameState == PLAYING) {
                        gameState = PAUSED;
                    }
                    else if (gameState == PAUSED) {
                        gameState = PLAYING;
                    }
                    else if (gameState == USERNAME_INPUT || gameState == TUTORIAL ||
                        gameState == HIGH_SCORES || gameState == SETTINGS) {
                        gameState = MAIN_MENU;
                    }
                    else if (gameState == LEVEL_SELECT) {
                        gameState = USERNAME_INPUT;
                    }
                }

                if (gameState == PLAYING && event.key.code == Keyboard::Space) {
                    helicopter.flap();
                    if (gameSettings.soundEnabled) {
                        flapSound.play();
                    }
                }
            }

            if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
                Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window));

                if (gameState == MAIN_MENU) {
                    if (playButton.isMouseOver(window)) {
                        gameState = USERNAME_INPUT;
                        playerNameInput = "";
                        nameInputText.setString("");
                    }
                    else if (scoresButton.isMouseOver(window)) {
                        gameState = HIGH_SCORES;
                    }
                    else if (settingsButton.isMouseOver(window)) {
                        gameState = SETTINGS;
                    }
                    else if (tutorialButton.isMouseOver(window)) {
                        gameState = TUTORIAL;
                    }
                    else if (exitButton.isMouseOver(window)) {
                        window.close();
                    }
                }
                else if (gameState == USERNAME_INPUT) {
                    if (continueButton.isMouseOver(window) && !playerNameInput.isEmpty()) {
                        currentPlayerName = playerNameInput;
                        gameState = LEVEL_SELECT;
                    }
                }
                else if (gameState == PAUSED) {
                    if (resumeButton.isMouseOver(window)) {
                        gameState = PLAYING;
                    }
                    else if (playButton.isMouseOver(window)) {
                        gameState = USERNAME_INPUT;
                        playerNameInput = "";
                        nameInputText.setString("");
                    }
                    else if (scoresButton.isMouseOver(window)) {
                        gameState = HIGH_SCORES;
                    }
                    else if (exitButton.isMouseOver(window)) {
                        window.close();
                    }
                }
                else if (gameState == LEVEL_SELECT) {
                    if (easyButton.isMouseOver(window)) {
                        setDifficulty(EASY);
                        gameState = PLAYING;
                        // Reset game
                        helicopter = Helicopter(helicopterTexture);
                        for (int i = 0; i < MAX_OBSTACLES; i++) {
                            obstacles[i].active = false;
                        }
                        activeObstacles = 5;
                        obstacleTimer = 0;
                        score = 0;
                        gameOver = false;
                    }
                    else if (mediumButton.isMouseOver(window)) {
                        setDifficulty(MEDIUM);
                        gameState = PLAYING;
                        // Reset game
                        helicopter = Helicopter(helicopterTexture);
                        for (int i = 0; i < MAX_OBSTACLES; i++) {
                            obstacles[i].active = false;
                        }
                        activeObstacles = 5;
                        obstacleTimer = 0;
                        score = 0;
                        gameOver = false;
                    }
                    else if (hardButton.isMouseOver(window)) {
                        setDifficulty(HARD);
                        gameState = PLAYING;
                        // Reset game
                        helicopter = Helicopter(helicopterTexture);
                        for (int i = 0; i < MAX_OBSTACLES; i++) {
                            obstacles[i].active = false;
                        }
                        activeObstacles = 5;
                        obstacleTimer = 0;
                        score = 0;
                        gameOver = false;
                    }
                    else if (backButton.isMouseOver(window)) {
                        gameState = USERNAME_INPUT;
                    }
                }
                else if (gameState == HIGH_SCORES) {
                    if (backButton.isMouseOver(window)) {
                        gameState = MAIN_MENU;
                    }
                }
                else if (gameState == GAME_OVER) {
                    if (playButton.isMouseOver(window)) {
                        gameState = USERNAME_INPUT;
                        playerNameInput = "";
                        nameInputText.setString("");
                    }
                    else if (exitButton.isMouseOver(window)) {
                        gameState = MAIN_MENU;
                    }
                }
                else if (gameState == TUTORIAL) {
                    if (backButton.isMouseOver(window)) {
                        gameState = MAIN_MENU;
                    }
                }
                else if (gameState == SETTINGS) {
                    if (backButton.isMouseOver(window)) {
                        gameState = MAIN_MENU;
                        saveSettings();
                    }
                }
            }
        }

        // Update
        if (gameState == PLAYING && !gameOver) {
            background.update(deltaTime);
            helicopter.update(deltaTime);
            particles.update(deltaTime);

            // Spawn obstacles
            if (++obstacleTimer >= OBSTACLE_FREQUENCY && activeObstacles < MAX_OBSTACLES) {
                ObstacleType type;
                if (powerupDist(gen) < 5) { // 5% chance for power-up
                    type = POWERUP;
                }
                else {
                    type = static_cast<ObstacleType>(obstacleTypeDist(gen) % 4); // 0-3 for regular obstacles
                }

                float y = (type == ROCK || type == TREE) ? 0.0f : static_cast<float>(heightDist(gen));

                for (int i = 0; i < MAX_OBSTACLES; i++) {
                    if (!obstacles[i].active) {
                        if (type == POWERUP) {
                            obstacles[i].create(type, static_cast<float>(WINDOW_WIDTH), y, powerupTexture);
                        }
                        else {
                            obstacles[i].create(type, static_cast<float>(WINDOW_WIDTH), y, obstacleTexture);
                        }
                        activeObstacles++;
                        break;
                    }
                }
                obstacleTimer = 0;
            }

            // Update obstacles and check collisions
            for (int i = 0; i < MAX_OBSTACLES; i++) {
                if (!obstacles[i].active) continue;

                obstacles[i].update(deltaTime);

                if (obstacles[i].type != CLOUD) {
                    if (obstacles[i].type == POWERUP) {
                        if (helicopter.getBounds().intersects(obstacles[i].getBounds())) {
                            obstacles[i].active = false;
                            activeObstacles--;
                            helicopter.applyPowerUp(obstacles[i].powerType);
                            if (gameSettings.soundEnabled) {
                                powerupSound.play();
                            }
                            particles.createExplosion(
                                Vector2f(
                                    obstacles[i].getBounds().left + obstacles[i].getBounds().width / 2.0f,
                                    obstacles[i].getBounds().top + obstacles[i].getBounds().height / 2.0f
                                ),
                                30
                            );
                        }
                    }
                    else if (helicopter.checkCollision(obstacles[i].getBounds())) {
                        gameOver = true;
                        gameState = GAME_OVER;
                        addHighScore(score / 10);
                        if (gameSettings.soundEnabled) {
                            crashSound.play();
                        }
                        particles.createExplosion(
                            Vector2f(
                                helicopter.getBounds().left + helicopter.getBounds().width / 2.0f,
                                helicopter.getBounds().top + helicopter.getBounds().height / 2.0f
                            ),
                            100
                        );
                        backgroundMusic.stop();
                    }
                }

                if (obstacles[i].isOffScreen()) {
                    obstacles[i].active = false;
                    activeObstacles--;
                }
            }

            score += helicopter.getScoreMultiplier();
        }

        // Update buttons based on current state
        if (gameState == MAIN_MENU) {
            playButton.update(window);
            scoresButton.update(window);
            settingsButton.update(window);
            tutorialButton.update(window);
            exitButton.update(window);
        }
        else if (gameState == USERNAME_INPUT) {
            continueButton.update(window);
        }
        else if (gameState == PAUSED) {
            resumeButton.update(window);
            playButton.update(window);
            scoresButton.update(window);
            exitButton.update(window);
        }
        else if (gameState == LEVEL_SELECT) {
            easyButton.update(window);
            mediumButton.update(window);
            hardButton.update(window);
            backButton.update(window);
        }
        else if (gameState == HIGH_SCORES || gameState == TUTORIAL || gameState == SETTINGS) {
            backButton.update(window);
        }
        else if (gameState == GAME_OVER) {
            playButton.update(window);
            exitButton.update(window);
        }

        // Rendering
        window.clear(Color(135, 206, 235)); // Sky blue

        if (gameState == PLAYING || gameState == PAUSED || gameState == GAME_OVER) {
            // Draw background
            background.draw(window);

            // Draw ground
            RectangleShape ground(Vector2f(static_cast<float>(WINDOW_WIDTH), 50.0f));
            ground.setPosition(0.0f, static_cast<float>(WINDOW_HEIGHT - 50));
            ground.setFillColor(Color(34, 139, 34)); // Green
            window.draw(ground);

            // Draw clouds first (background)
            for (int i = 0; i < MAX_OBSTACLES; i++) {
                if (obstacles[i].active && obstacles[i].type == CLOUD) {
                    obstacles[i].draw(window);
                }
            }

            // Draw other obstacles
            for (int i = 0; i < MAX_OBSTACLES; i++) {
                if (obstacles[i].active && obstacles[i].type != CLOUD) {
                    obstacles[i].draw(window);
                }
            }

            // Draw particles
            particles.draw(window);

            // Draw helicopter
            helicopter.draw(window);

            // Draw UI
            scoreText.setString("Score: " + to_string(score / 10) + " - " + currentPlayerName +
                " (x" + to_string(helicopter.getScoreMultiplier()) + ")");
            window.draw(scoreText);
        }

        if (gameState == MAIN_MENU) {
            window.draw(titleText);
            playButton.draw(window);
            scoresButton.draw(window);
            settingsButton.draw(window);
            tutorialButton.draw(window);
            exitButton.draw(window);
        }
        else if (gameState == USERNAME_INPUT) {
            window.clear(Color(70, 70, 70));

            Text title;
            title.setFont(font);
            title.setString("Enter Player Name");
            title.setCharacterSize(48);
            title.setFillColor(Color::White);
            title.setPosition(WINDOW_WIDTH / 2.0f - title.getLocalBounds().width / 2.0f, 100.0f);
            window.draw(title);

            window.draw(namePromptText);

            RectangleShape inputBox(Vector2f(300.0f, 40.0f));
            inputBox.setPosition(WINDOW_WIDTH / 2.0f - 150.0f, 250.0f);
            inputBox.setFillColor(Color::Black);
            inputBox.setOutlineThickness(2.0f);
            inputBox.setOutlineColor(Color::White);
            window.draw(inputBox);

            window.draw(nameInputText);
            continueButton.draw(window);

            if (playerNameInput.isEmpty()) {
                Text hintText;
                hintText.setFont(font);
                hintText.setString("Please enter your name to continue");
                hintText.setCharacterSize(20);
                hintText.setFillColor(Color::Red);
                hintText.setPosition(WINDOW_WIDTH / 2.0f - hintText.getLocalBounds().width / 2.0f, 380.0f);
                window.draw(hintText);
            }
        }
        else if (gameState == PAUSED) {
            window.draw(pauseText);
            resumeButton.draw(window);
            playButton.draw(window);
            scoresButton.draw(window);
            exitButton.draw(window);
        }
        else if (gameState == LEVEL_SELECT) {
            Text levelText;
            levelText.setFont(font);
            levelText.setString("Select Difficulty");
            levelText.setCharacterSize(36);
            levelText.setFillColor(Color::White);
            levelText.setPosition(WINDOW_WIDTH / 2.0f - levelText.getLocalBounds().width / 2.0f, 100.0f);
            window.draw(levelText);

            easyButton.draw(window);
            mediumButton.draw(window);
            hardButton.draw(window);
            backButton.draw(window);
        }
        else if (gameState == HIGH_SCORES) {
            Text scoresTitle;
            scoresTitle.setFont(font);
            scoresTitle.setString("High Scores");
            scoresTitle.setCharacterSize(36);
            scoresTitle.setFillColor(Color::White);
            scoresTitle.setPosition(WINDOW_WIDTH / 2.0f - scoresTitle.getLocalBounds().width / 2.0f, 50.0f);
            window.draw(scoresTitle);

            for (int i = 0; i < numHighScores && i < MAX_HIGH_SCORES; i++) {
                Text scoreEntry;
                scoreEntry.setFont(font);

                string difficultyStr;
                switch (highScores[i].difficulty) {
                case EASY: difficultyStr = "Easy"; break;
                case MEDIUM: difficultyStr = "Medium"; break;
                case HARD: difficultyStr = "Hard"; break;
                }

                scoreEntry.setString(to_string(i + 1) + ". " + highScores[i].name + " - " +
                    to_string(highScores[i].score) + " (" + difficultyStr + ")");
                scoreEntry.setCharacterSize(24);
                scoreEntry.setFillColor(Color::White);
                scoreEntry.setPosition(WINDOW_WIDTH / 2.0f - 150.0f, 120.0f + i * 30.0f);
                window.draw(scoreEntry);
            }

            backButton.draw(window);
        }
        else if (gameState == GAME_OVER) {
            Text gameOverText;
            gameOverText.setFont(font);
            gameOverText.setString("Game Over! Final Score: " + to_string(score / 10));
            gameOverText.setCharacterSize(36);
            gameOverText.setFillColor(Color::White);
            gameOverText.setPosition(WINDOW_WIDTH / 2.0f - gameOverText.getLocalBounds().width / 2.0f, 150.0f);
            window.draw(gameOverText);

            playButton.draw(window);
            exitButton.draw(window);
        }
        else if (gameState == TUTORIAL) {
            window.clear(Color(70, 70, 70));

            Text tutorialTitle;
            tutorialTitle.setFont(font);
            tutorialTitle.setString("How to Play");
            tutorialTitle.setCharacterSize(48);
            tutorialTitle.setFillColor(Color::White);
            tutorialTitle.setPosition(WINDOW_WIDTH / 2.0f - tutorialTitle.getLocalBounds().width / 2.0f, 50.0f);
            window.draw(tutorialTitle);

            Text tutorialText;
            tutorialText.setFont(font);
            tutorialText.setString(
                "CONTROLS:\n"
                "Press SPACE to make the helicopter ascend\n"
                "Press ESC to pause the game\n\n"
                "OBJECTIVE:\n"
                "Avoid obstacles and try to survive as long as possible\n"
                "Each second survived gives you 1 point\n\n"
                "POWER-UPS:\n"
                "Blue Orb - Shield (protects from one hit)\n"
                "Green Orb - Score Multiplier (doubles points)\n"
                "Red Orb - Speed Boost (increases flap power)\n"
                "Yellow Orb - Invincibility (no collisions)\n\n"
                "DIFFICULTY LEVELS:\n"
                "Easy - Slower obstacles, more forgiving physics\n"
                "Medium - Balanced challenge\n"
                "Hard - Faster obstacles, more difficult physics"
            );
            tutorialText.setCharacterSize(24);
            tutorialText.setFillColor(Color::White);
            tutorialText.setPosition(50.0f, 120.0f);
            window.draw(tutorialText);

            backButton.draw(window);
        }
        else if (gameState == SETTINGS) {
            window.clear(Color(70, 70, 70));

            Text settingsTitle;
            settingsTitle.setFont(font);
            settingsTitle.setString("Settings");
            settingsTitle.setCharacterSize(48);
            settingsTitle.setFillColor(Color::White);
            settingsTitle.setPosition(WINDOW_WIDTH / 2.0f - settingsTitle.getLocalBounds().width / 2.0f, 50.0f);
            window.draw(settingsTitle);

            // Sound and music toggle buttons would be implemented here
            // Volume sliders would be implemented here

            backButton.draw(window);
        }

        window.display();
    }

    return 0;
}