#include <SFML/Graphics.hpp>

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "Helicopter Game");
    

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            // Added 'esc' key to close window
            if (event.type == sf::Event::Closed || sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
            {
                window.close();
            }
        }
    }

    return 0;
}