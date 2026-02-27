#ifndef ECE_LASERBLAST_HPP
#define ECE_LASERBLAST_HPP

#include <SFML/Graphics.hpp>

class ECE_LaserBlast
{
public:
    ECE_LaserBlast(float xStart, float yStart, float speed);
    void draw(sf::RenderWindow &window);
    void update(float dt);
    bool isOffScreen(int boundary, bool downwards) const;
    sf::FloatRect getBounds() const;

private:
    sf::Texture texture;
    sf::Sprite sprite;
    sf::RectangleShape laserRect; // Fallback rectangle if texture fails
    float speed;
    bool useTexture; // Flag to determine which rendering method to use
};

#endif