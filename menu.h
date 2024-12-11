#include <SFML/Graphics.hpp>
#include "Simulation.h"
#include <ostream>
#include <fstream>
using namespace sf;
// laptop scrren size
const int window_width = 1920;
const int window_height = 1050;
class Menu
{
public:
    int currentOption;
    Sprite background;
    Texture texture;
    Text statement;
    sf::RenderWindow window;
    Simulation s;
    Menu() : window(sf::VideoMode(window_width, window_height), "girti hoi bdewar ko iek dhka aur"), s(window_width, window_height)
    {

        currentOption = 0;
        background.setScale(static_cast<float>(window_width) / texture.getSize().x, static_cast<float>(window_height) / texture.getSize().y);
    }
    void display_menu()
    {
        Font font;
        font.loadFromFile("Arial.ttf");

        statement.setFont(font);
        statement.setCharacterSize(50);
        statement.setStyle(sf::Text::Bold);
        statement.setFillColor(sf::Color::Black);
        statement.setPosition(350, 350 + 100);

        statement.setString("Dekhao");

        while (window.isOpen())
        {
            sf::Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                    window.close();
            }
            if (Keyboard::isKeyPressed(Keyboard::Enter))
                Choose(window);

            window.clear(sf::Color::White);
            window.draw(background);
            window.draw(statement);

            window.display();
        }
    }
    void Choose(RenderWindow &window)
    {

        if (currentOption == 0)
            s.start(window);
    }
};
