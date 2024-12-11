#include <unordered_map>
#include <SFML/Graphics.hpp>
enum VehicelType
{
    EMERGENCY,
    REGULAR,
    HEAVY,
    OutOfOrder,
};
using namespace std;
using namespace sf;
class TextureManager
{
public:
    static sf::Texture &getTexture(const std::string &filename)
    {
        // Static map to store textures
        static std::unordered_map<std::string, sf::Texture> textures;

        // Check if the texture is already loaded
        if (textures.find(filename) == textures.end())
        {
            // Load the texture if not already loaded
            sf::Texture texture;
            if (!texture.loadFromFile(filename))
                throw std::runtime_error("Failed to load texture: " + filename);

            textures[filename] = std::move(texture);
        }

        // Return a reference to the texture
        return textures[filename];
    }
};

class Vehicle // for checking:the car will only move from east road to west road currently
{
public:
    Sprite sprite;
    float speed;
    VehicelType type;
    Vehicle(const string &imgPath, float speed, int x, int y, VehicelType type) : speed(speed)
    {
        sf::Texture &tex = TextureManager::getTexture(imgPath);
        sprite.setTexture(tex);
        sprite.setPosition(x, y);
        sprite.setScale(0.75, 0.75);
        // this.speed = speed;
        this->type = type;
    }
    float getCurrentSpeed()
    {
        return this->speed;
    }
    bool isColliding(const Vehicle &v)
    {

        return sprite.getGlobalBounds().intersects(v.sprite.getGlobalBounds());
    }
    void move(string way, float delta_time)
    {
        float delta_x = 0, delta_y = 0;
        if (way == "east")
            delta_x = -1;

        else if (way == "west")
            delta_x = 1;
        else if (way == "north")
            delta_y = 1;
        else if (way == "south")
            delta_y = -1;

        delta_x *= speed;
        delta_y *= speed;

        sprite.move(delta_x, delta_y);
    }
    sf::Vector2f getSize()
    {
        return sprite.getGlobalBounds().getSize();
    }
};