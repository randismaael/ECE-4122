#include "ECE_Buzzy.hpp"
#include <iostream>

ECE_Buzzy::ECE_Buzzy(float xStart, float yStart)
    : speed(300.f), canShoot(true), shootCooldown(0.2f), timeSinceLastShot(0.0f)
{
    if (!texture.loadFromFile("graphics/Buzzy_blue.png"))
        std::cout << "Error loading Buzzy_blue.png\n";
    sprite.setTexture(texture);
    sprite.setScale(0.24f, 0.24f); // adjust as needed for Buzzy size
    sprite.setPosition(xStart, yStart);
}

void ECE_Buzzy::draw(sf::RenderWindow &window)
{
    window.draw(sprite);
    for (auto &laser : lasers)
        laser.draw(window);
}

void ECE_Buzzy::move(float dx)
{
    sprite.move(dx, 0);
    float xLeft = sprite.getPosition().x;
    float xRight = xLeft + sprite.getGlobalBounds().width;
    if (xLeft < 0)
        sprite.setPosition(0, sprite.getPosition().y);
    if (xRight > 1152)
        sprite.setPosition(1152 - sprite.getGlobalBounds().width, sprite.getPosition().y);
}

void ECE_Buzzy::shoot()
{
    if (canShoot)
    {
        const float laserWidth = 10.0f; // Set laser width to an appropriate value
        float x = sprite.getPosition().x + sprite.getGlobalBounds().width / 2.f - laserWidth / 2.f;
        float y = sprite.getPosition().y + sprite.getGlobalBounds().height;
        lasers.emplace_back(x, y, 500.0f);
        canShoot = false;
        timeSinceLastShot = 0.0f;
    }
}

void ECE_Buzzy::update(float dt)
{
    timeSinceLastShot += dt;
    if (timeSinceLastShot > shootCooldown)
        canShoot = true;
    for (auto it = lasers.begin(); it != lasers.end();)
    {
        it->update(dt);
        if (it->isOffScreen(1030, true))
            it = lasers.erase(it);
        else
            ++it;
    }
}

std::list<ECE_LaserBlast> &ECE_Buzzy::getLasers()
{
    return lasers;
}

sf::FloatRect ECE_Buzzy::getBounds() const
{
    return sprite.getGlobalBounds();
}

sf::Vector2f ECE_Buzzy::getPosition() const
{
    return sprite.getPosition();
}
