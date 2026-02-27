#include "ECE_LaserBlast.hpp"
#include <iostream>

ECE_LaserBlast::ECE_LaserBlast(float xStart, float yStart, float speed)
    : speed(speed), useTexture(false)
{
    // Try to load texture first
    if (texture.loadFromFile("graphics/LaserBlast.png"))
    {
        sprite.setTexture(texture);
        sprite.setScale(0.1f, 0.5f);
        useTexture = true;
    }
    // Set position for both sprite and rectangle
    sprite.setPosition(xStart, yStart);
    laserRect.setPosition(xStart, yStart);
}

void ECE_LaserBlast::draw(sf::RenderWindow &window)
{
    if (useTexture)
        window.draw(sprite);
    else
        window.draw(laserRect);
}

void ECE_LaserBlast::update(float dt)
{
    if (useTexture)
        sprite.move(0, speed * dt);
    else
        laserRect.move(0, speed * dt);
}

bool ECE_LaserBlast::isOffScreen(int boundary, bool downwards) const
{
    float y = useTexture ? sprite.getPosition().y : laserRect.getPosition().y;
    if (downwards)
        return y > boundary;
    else
    {
        float height = useTexture ? sprite.getGlobalBounds().height : laserRect.getSize().y;
        return y < -height;
    }
}

sf::FloatRect ECE_LaserBlast::getBounds() const
{
    return useTexture ? sprite.getGlobalBounds() : laserRect.getGlobalBounds();
}