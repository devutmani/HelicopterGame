#include <SFML/Graphics.hpp>
#include <random>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>

// Game Constants
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
float GRAVITY = 0.3f;
float FLAP_FORCE = -7.0f;
float OBSTACLE_SPEED = 3.0f;
int OBSTACLE_FREQUENCY = 150;
const int MAX_OBSTACLES = 20;
const int MAX_HIGH_SCORES = 10;

// Game States
enum GameState {
    MAIN_MENU,
    USERNAME_INPUT,
    LEVEL_SELECT,
    PLAYING,
    PAUSED,
    GAME_OVER,
    HIGH_SCORES
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
    NONE
};

// High score structure
struct HighScore {
    std::string name;
    int score;
    Difficulty difficulty;
};

HighScore highScores[MAX_HIGH_SCORES];
int numHighScores = 0;
std::string currentPlayerName = "Player";

class Button {
private:
    sf::RectangleShape shape;
    sf::Text text;
    sf::Color idleColor;
    sf::Color hoverColor;
    sf::Color activeColor;

public:
    Button(float x, float y, float width, float height,
        sf::Font& font, std::string btnText,
        sf::Color idle, sf::Color hover, sf::Color active) {
        shape.setPosition(sf::Vector2f(x, y));
        shape.setSize(sf::Vector2f(width, height));

        text.setFont(font);
        text.setString(btnText);
        text.setFillColor(sf::Color::White);
        text.setCharacterSize(24);
        text.setPosition(
            x + (width / 2.f) - text.getLocalBounds().width / 2.f,
            y + (height / 2.f) - text.getLocalBounds().height / 2.f
        );

        idleColor = idle;
        hoverColor = hover;
        activeColor = active;

        shape.setFillColor(idleColor);
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
        window.draw(text);
    }

    bool isMouseOver(sf::RenderWindow& window) {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        return shape.getGlobalBounds().contains(mousePos);
    }

    void update(sf::RenderWindow& window) {
        if (isMouseOver(window)) {
            shape.setFillColor(hoverColor);
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                shape.setFillColor(activeColor);
            }
        }
        else {
            shape.setFillColor(idleColor);
        }
    }
};

class Obstacle {
public:
    ObstacleType type = NONE;
    bool active = false;
    sf::RectangleShape shape;
    sf::CircleShape leaves; // For trees
    float yPos = 0;

    void create(ObstacleType t, float x, float y) {
        type = t;
        active = true;
        yPos = y;

        switch (type) {
        case ROCK:
            shape.setSize(sf::Vector2f(60, 60));
            shape.setPosition(x, WINDOW_HEIGHT - 60);
            shape.setFillColor(sf::Color(139, 69, 19)); // Brown
            break;
        case BIRD:
            shape.setSize(sf::Vector2f(40, 30));
            shape.setPosition(x, y);
            shape.setFillColor(sf::Color(255, 165, 0)); // Orange
            break;
        case TREE:
            shape.setSize(sf::Vector2f(20, 60)); // Trunk
            shape.setPosition(x, WINDOW_HEIGHT - 60 - 20);
            shape.setFillColor(sf::Color(139, 69, 19)); // Brown

            leaves.setRadius(30); // Tree top
            leaves.setPosition(x - 10, WINDOW_HEIGHT - 60 - 20 - 30);
            leaves.setFillColor(sf::Color(34, 139, 34)); // Green
            break;
        case CLOUD:
            shape.setSize(sf::Vector2f(80, 40));
            shape.setPosition(x, y);
            shape.setFillColor(sf::Color(240, 240, 240)); // Light gray
            break;
        case NONE:
            break;
        }
    }

    void update() {
        if (!active) return;

        float speed = OBSTACLE_SPEED;
        if (type == BIRD) speed *= 1.2f;
        if (type == CLOUD) speed *= 0.7f;

        shape.move(-speed, 0);
        if (type == TREE) leaves.move(-speed, 0);
    }

    void draw(sf::RenderWindow& window) const {
        if (!active) return;

        window.draw(shape);
        if (type == TREE) window.draw(leaves);
    }

    bool isOffScreen() const {
        return active && (shape.getPosition().x + shape.getSize().x < 0);
    }

    sf::FloatRect getBounds() const {
        if (type == TREE) return leaves.getGlobalBounds();
        return shape.getGlobalBounds();
    }
};

class Helicopter {
private:
    sf::RectangleShape body;
    sf::RectangleShape rotor;
    sf::Vector2f velocity;
    float rotationAngle;

public:
    Helicopter() {
        // Initialize helicopter shape
        body.setSize(sf::Vector2f(50, 20));
        body.setFillColor(sf::Color::Red);
        body.setPosition(100, WINDOW_HEIGHT / 2);
        body.setOrigin(body.getSize().x / 2, body.getSize().y / 2);

        rotor.setSize(sf::Vector2f(60, 5));
        rotor.setFillColor(sf::Color::White);
        rotor.setOrigin(rotor.getSize().x / 2, rotor.getSize().y / 2);

        velocity = sf::Vector2f(0, 0);
        rotationAngle = 0;
    }

    void flap() {
        velocity.y = FLAP_FORCE;
        rotationAngle = -20; // Tilt up when flapping
    }

    void update() {
        // Apply gravity
        velocity.y += GRAVITY;

        // Update position
        body.move(0, velocity.y);
        rotor.setPosition(body.getPosition());

        // Rotate back to horizontal gradually
        if (rotationAngle < 0) rotationAngle += 1.0f;
        else if (velocity.y > 0) rotationAngle = std::min(rotationAngle + 0.5f, 20.0f);

        body.setRotation(rotationAngle);
        rotor.rotate(10); // Spin the rotor

        // Keep helicopter within screen bounds
        sf::Vector2f position = body.getPosition();
        if (position.y < 0) {
            position.y = 0;
            velocity.y = 0;
        }
        else if (position.y > WINDOW_HEIGHT - 50) { // 50 is ground height
            position.y = WINDOW_HEIGHT - 50;
            velocity.y = 0;
        }
        body.setPosition(position);
    }

    void draw(sf::RenderWindow& window) const {
        window.draw(rotor);
        window.draw(body);
    }

    sf::FloatRect getBounds() const {
        return body.getGlobalBounds();
    }
};

void loadHighScores() {
    std::ifstream file("highscores.txt");
    if (file.is_open()) {
        numHighScores = 0;
        while (numHighScores < MAX_HIGH_SCORES) {
            int difficultyValue;
            file >> highScores[numHighScores].name
                >> highScores[numHighScores].score
                >> difficultyValue;

            if (file.fail()) break;  // Stop if reading fails

            highScores[numHighScores].difficulty = static_cast<Difficulty>(difficultyValue);
            numHighScores++;
        }
        file.close();
    }
}

void saveHighScores() {
    std::ofstream file("highscores.txt");
    if (file.is_open()) {
        for (int i = 0; i < numHighScores; i++) {
            file << highScores[i].name << " "
                << highScores[i].score << " "
                << static_cast<int>(highScores[i].difficulty) << "\n";
        }
        file.close();
    }
}

void addHighScore(int score) {
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

    std::sort(highScores, highScores + numHighScores, [](const HighScore& a, const HighScore& b) {
        return a.score > b.score;
        });

    saveHighScores();
}

void setDifficulty(Difficulty difficulty) {
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
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Helicopter Game");
    window.setFramerateLimit(60);

    // Load font
    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        std::cerr << "Failed to load font. Using default font." << std::endl;
    }

    // Load high scores
    loadHighScores();

    // Game objects
    Helicopter helicopter;
    Obstacle obstacles[MAX_OBSTACLES];
    int activeObstacles = 0;

    // Random number generation
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> heightDist(50, WINDOW_HEIGHT - 150);
    std::uniform_int_distribution<> cloudHeightDist(50, 200);
    std::uniform_int_distribution<> obstacleTypeDist(0, 3);

    // Game state
    GameState gameState = MAIN_MENU;
    int obstacleTimer = 0;
    int score = 0;
    bool gameOver = false;
    sf::String playerNameInput;
    bool nameEntered = false;

    // UI Elements
    sf::Text titleText;
    titleText.setFont(font);
    titleText.setString("Helicopter Game");
    titleText.setCharacterSize(48);
    titleText.setFillColor(sf::Color::White);
    titleText.setPosition(WINDOW_WIDTH / 2 - titleText.getLocalBounds().width / 2, 50);

    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(10, 10);

    sf::Text pauseText;
    pauseText.setFont(font);
    pauseText.setString("PAUSED");
    pauseText.setCharacterSize(48);
    pauseText.setFillColor(sf::Color::White);
    pauseText.setPosition(WINDOW_WIDTH / 2 - pauseText.getLocalBounds().width / 2, 150);

    // Username input elements
    sf::Text namePromptText;
    namePromptText.setFont(font);
    namePromptText.setString("Enter your name:");
    namePromptText.setCharacterSize(30);
    namePromptText.setFillColor(sf::Color::White);
    namePromptText.setPosition(WINDOW_WIDTH / 2 - namePromptText.getLocalBounds().width / 2, 200);

    sf::Text nameInputText;
    nameInputText.setFont(font);
    nameInputText.setString("");
    nameInputText.setCharacterSize(30);
    nameInputText.setFillColor(sf::Color::White);
    nameInputText.setPosition(WINDOW_WIDTH / 2 - 100, 250);

    // Buttons
    Button playButton(WINDOW_WIDTH / 2 - 100, 200, 200, 50, font, "Play Game",
        sf::Color::Blue, sf::Color::Cyan, sf::Color::Green);
    Button levelButton(WINDOW_WIDTH / 2 - 100, 270, 200, 50, font, "Game Level",
        sf::Color::Blue, sf::Color::Cyan, sf::Color::Green);
    Button scoresButton(WINDOW_WIDTH / 2 - 100, 340, 200, 50, font, "High Scores",
        sf::Color::Blue, sf::Color::Cyan, sf::Color::Green);
    Button exitButton(WINDOW_WIDTH / 2 - 100, 410, 200, 50, font, "Exit Game",
        sf::Color::Blue, sf::Color::Cyan, sf::Color::Green);
    Button resumeButton(WINDOW_WIDTH / 2 - 100, 200, 200, 50, font, "Resume",
        sf::Color::Blue, sf::Color::Cyan, sf::Color::Green);
    Button easyButton(WINDOW_WIDTH / 2 - 100, 200, 200, 50, font, "Easy",
        sf::Color::Blue, sf::Color::Cyan, sf::Color::Green);
    Button mediumButton(WINDOW_WIDTH / 2 - 100, 270, 200, 50, font, "Medium",
        sf::Color::Blue, sf::Color::Cyan, sf::Color::Green);
    Button hardButton(WINDOW_WIDTH / 2 - 100, 340, 200, 50, font, "Hard",
        sf::Color::Blue, sf::Color::Cyan, sf::Color::Green);
    Button backButton(WINDOW_WIDTH / 2 - 100, 410, 200, 50, font, "Back",
        sf::Color::Blue, sf::Color::Cyan, sf::Color::Green);
    Button continueButton(WINDOW_WIDTH / 2 - 100, 320, 200, 50, font, "Continue",
        sf::Color::Blue, sf::Color::Cyan, sf::Color::Green);

    // Create initial clouds
    for (int i = 0; i < 5 && activeObstacles < MAX_OBSTACLES; i++) {
        obstacles[activeObstacles++].create(CLOUD, i * 200, cloudHeightDist(gen));
    }

    // Game loop
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            // Handle text input for username
            if (gameState == USERNAME_INPUT) {
                if (event.type == sf::Event::TextEntered) {
                    if (event.text.unicode == '\b') { // Backspace
                        if (!playerNameInput.isEmpty()) {
                            playerNameInput.erase(playerNameInput.getSize() - 1);
                        }
                    }
                    else if (event.text.unicode < 128 && event.text.unicode != '\r') {
                        playerNameInput += event.text.unicode;
                    }
                    nameInputText.setString(playerNameInput);
                }
            }

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    if (gameState == PLAYING) {
                        gameState = PAUSED;
                    }
                    else if (gameState == PAUSED) {
                        gameState = PLAYING;
                    }
                    else if (gameState == USERNAME_INPUT) {
                        gameState = MAIN_MENU;
                    }
                }

                if (gameState == PLAYING && event.key.code == sf::Keyboard::Space) {
                    helicopter.flap();
                }
            }

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

                if (gameState == MAIN_MENU) {
                    if (playButton.isMouseOver(window)) {
                        gameState = USERNAME_INPUT;
                        playerNameInput = "";
                        nameInputText.setString("");
                    }
                    else if (levelButton.isMouseOver(window)) {
                        gameState = LEVEL_SELECT;
                    }
                    else if (scoresButton.isMouseOver(window)) {
                        gameState = HIGH_SCORES;
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
                    else if (levelButton.isMouseOver(window)) {
                        gameState = LEVEL_SELECT;
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
                        helicopter = Helicopter();
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
                        helicopter = Helicopter();
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
                        helicopter = Helicopter();
                        for (int i = 0; i < MAX_OBSTACLES; i++) {
                            obstacles[i].active = false;
                        }
                        activeObstacles = 5;
                        obstacleTimer = 0;
                        score = 0;
                        gameOver = false;
                    }
                    else if (backButton.isMouseOver(window)) {
                        gameState = MAIN_MENU;
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
            }
        }

        // Update
        if (gameState == PLAYING && !gameOver) {
            helicopter.update();

            if (++obstacleTimer >= OBSTACLE_FREQUENCY && activeObstacles < MAX_OBSTACLES) {
                ObstacleType type = static_cast<ObstacleType>(obstacleTypeDist(gen));
                float y = (type == ROCK || type == TREE) ? 0 : heightDist(gen);

                for (int i = 0; i < MAX_OBSTACLES; i++) {
                    if (!obstacles[i].active) {
                        obstacles[i].create(type, WINDOW_WIDTH, y);
                        activeObstacles++;
                        break;
                    }
                }
                obstacleTimer = 0;
            }

            for (int i = 0; i < MAX_OBSTACLES; i++) {
                if (!obstacles[i].active) continue;

                obstacles[i].update();

                if (obstacles[i].type != CLOUD &&
                    helicopter.getBounds().intersects(obstacles[i].getBounds())) {
                    gameOver = true;
                    gameState = GAME_OVER;
                    addHighScore(score / 10);
                }

                if (obstacles[i].isOffScreen()) {
                    obstacles[i].active = false;
                    activeObstacles--;
                }
            }

            score++;
        }

        // Update buttons based on current state
        if (gameState == MAIN_MENU) {
            playButton.update(window);
            levelButton.update(window);
            scoresButton.update(window);
            exitButton.update(window);
        }
        else if (gameState == USERNAME_INPUT) {
            continueButton.update(window);
        }
        else if (gameState == PAUSED) {
            resumeButton.update(window);
            playButton.update(window);
            levelButton.update(window);
            scoresButton.update(window);
            exitButton.update(window);
        }
        else if (gameState == LEVEL_SELECT) {
            easyButton.update(window);
            mediumButton.update(window);
            hardButton.update(window);
            backButton.update(window);
        }
        else if (gameState == HIGH_SCORES) {
            backButton.update(window);
        }
        else if (gameState == GAME_OVER) {
            playButton.update(window);
            exitButton.update(window);
        }

        // Rendering
        window.clear(sf::Color(135, 206, 235)); // Sky blue

        if (gameState == PLAYING || gameState == PAUSED || gameState == GAME_OVER) {
            // Draw ground
            sf::RectangleShape ground(sf::Vector2f(WINDOW_WIDTH, 50));
            ground.setPosition(0, WINDOW_HEIGHT - 50);
            ground.setFillColor(sf::Color(34, 139, 34)); // Green
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

            // Draw helicopter
            helicopter.draw(window);

            // Draw UI
            scoreText.setString("Score: " + std::to_string(score / 10) + " - " + currentPlayerName);
            window.draw(scoreText);
        }

        if (gameState == MAIN_MENU) {
            window.draw(titleText);
            playButton.draw(window);
            levelButton.draw(window);
            scoresButton.draw(window);
            exitButton.draw(window);
        }
        else if (gameState == USERNAME_INPUT) {
            window.clear(sf::Color(70, 70, 70));

            sf::Text title;
            title.setFont(font);
            title.setString("Enter Player Name");
            title.setCharacterSize(48);
            title.setFillColor(sf::Color::White);
            title.setPosition(WINDOW_WIDTH / 2 - title.getLocalBounds().width / 2, 100);
            window.draw(title);

            window.draw(namePromptText);

            sf::RectangleShape inputBox(sf::Vector2f(300, 40));
            inputBox.setPosition(WINDOW_WIDTH / 2 - 150, 250);
            inputBox.setFillColor(sf::Color::Black);
            inputBox.setOutlineThickness(2);
            inputBox.setOutlineColor(sf::Color::White);
            window.draw(inputBox);

            window.draw(nameInputText);
            continueButton.draw(window);

            if (playerNameInput.isEmpty()) {
                sf::Text hintText;
                hintText.setFont(font);
                hintText.setString("Please enter your name to continue");
                hintText.setCharacterSize(20);
                hintText.setFillColor(sf::Color::Red);
                hintText.setPosition(WINDOW_WIDTH / 2 - hintText.getLocalBounds().width / 2, 380);
                window.draw(hintText);
            }
        }
        else if (gameState == PAUSED) {
            window.draw(pauseText);
            resumeButton.draw(window);
            playButton.draw(window);
            levelButton.draw(window);
            scoresButton.draw(window);
            exitButton.draw(window);
        }
        else if (gameState == LEVEL_SELECT) {
            sf::Text levelText;
            levelText.setFont(font);
            levelText.setString("Select Difficulty");
            levelText.setCharacterSize(36);
            levelText.setFillColor(sf::Color::White);
            levelText.setPosition(WINDOW_WIDTH / 2 - levelText.getLocalBounds().width / 2, 100);
            window.draw(levelText);

            easyButton.draw(window);
            mediumButton.draw(window);
            hardButton.draw(window);
            backButton.draw(window);
        }
        else if (gameState == HIGH_SCORES) {
            sf::Text scoresTitle;
            scoresTitle.setFont(font);
            scoresTitle.setString("High Scores");
            scoresTitle.setCharacterSize(36);
            scoresTitle.setFillColor(sf::Color::White);
            scoresTitle.setPosition(WINDOW_WIDTH / 2 - scoresTitle.getLocalBounds().width / 2, 50);
            window.draw(scoresTitle);

            for (int i = 0; i < numHighScores && i < MAX_HIGH_SCORES; i++) {
                sf::Text scoreEntry;
                scoreEntry.setFont(font);

                std::string difficultyStr;
                switch (highScores[i].difficulty) {
                case EASY: difficultyStr = "Easy"; break;
                case MEDIUM: difficultyStr = "Medium"; break;
                case HARD: difficultyStr = "Hard"; break;
                }

                scoreEntry.setString(std::to_string(i + 1) + ". " + highScores[i].name + " - " +
                    std::to_string(highScores[i].score) + " (" + difficultyStr + ")");
                scoreEntry.setCharacterSize(24);
                scoreEntry.setFillColor(sf::Color::White);
                scoreEntry.setPosition(WINDOW_WIDTH / 2 - 150, 120 + i * 30);
                window.draw(scoreEntry);
            }

            backButton.draw(window);
        }
        else if (gameState == GAME_OVER) {
            sf::Text gameOverText;
            gameOverText.setFont(font);
            gameOverText.setString("Game Over! Final Score: " + std::to_string(score / 10));
            gameOverText.setCharacterSize(36);
            gameOverText.setFillColor(sf::Color::White);
            gameOverText.setPosition(WINDOW_WIDTH / 2 - gameOverText.getLocalBounds().width / 2, 150);
            window.draw(gameOverText);

            playButton.draw(window);
            exitButton.draw(window);
        }

        window.display();
    }

    return 0;
}