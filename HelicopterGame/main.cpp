#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "Amazing SFML Window");
    window.setFramerateLimit(60);

    sf::Font font;
    font.loadFromFile("arial.ttf");

    sf::Text text;
    text.setFont(font);
    text.setString("Hello, SFML!");
    text.setCharacterSize(50);
    text.setFillColor(sf::Color::Green);
    text.setPosition(250, 250);

    sf::CircleShape circle(100.f);
    circle.setFillColor(sf::Color::Blue);
    circle.setPosition(300, 200);

    sf::Music music;
    music.openFromFile("music.ogg");
    music.play();

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        window.draw(circle);
        window.draw(text);
        window.display();
    }

    return 0;
}