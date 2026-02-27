#include "ECE_Enemy.hpp"
#include <iostream>

ECE_Enemy::ECE_Enemy(const std::string &textureFile, float xStart, float yStart)
    : speed(60.f), canShoot(true), shootCooldown(6.5f), timeSinceLastShot(0.0f)
{
    if (!texture.loadFromFile(textureFile))
        std::cout << "Error loading " << textureFile << "\n";
    sprite.setTexture(texture);
    sprite.setScale(0.17f, 0.17f); // adjust for enemy size
    sprite.setTexture(texture);
    sprite.setTexture(texture);
    if (textureFile.find("clemson_tigers") != std::string::npos)
    {
        sprite.setScale(0.06f, 0.06f); // for 872x917 images, make it very small
    }
    else
    {
        sprite.setScale(0.20, 0.20f); // for 288x302, larger scale
    }
    sprite.setPosition(xStart, yStart);

    sprite.setPosition(xStart, yStart);
}

void ECE_Enemy::draw(sf::RenderWindow &window)
{
    window.draw(sprite);
    for (auto &laser : lasers)
        laser.draw(window);
}

void ECE_Enemy::move(float dx, float dy)
{
    sprite.move(dx, dy);
}

void ECE_Enemy::shoot()
{
    if (canShoot)
    {
        float x = sprite.getPosition().x + sprite.getGlobalBounds().width / 2.f;
        float y = sprite.getPosition().y;
        lasers.emplace_back(x, y, -350.0f);
        canShoot = false;
        timeSinceLastShot = 0.0f;
    }
}

void ECE_Enemy::update(float dt)
{
    timeSinceLastShot += dt;
    if (timeSinceLastShot > shootCooldown)
        canShoot = true;
    for (auto it = lasers.begin(); it != lasers.end();)
    {
        it->update(dt);
        if (it->isOffScreen(0, false))
            it = lasers.erase(it);
        else
            ++it;
    }
}

std::list<ECE_LaserBlast> &ECE_Enemy::getLasers()
{
    return lasers;
}

sf::FloatRect ECE_Enemy::getBounds() const
{
    return sprite.getGlobalBounds();
}

sf::Vector2f ECE_Enemy::getPosition() const
{
    return sprite.getPosition();
}
