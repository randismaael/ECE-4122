#pragma once
#include <SFML/Graphics.hpp>
#include <list>
#include "ECE_LaserBlast.hpp"

class ECE_Buzzy
{
public:
    ECE_Buzzy(float xStart, float yStart);
    void draw(sf::RenderWindow &window);
    void move(float dx);
    void shoot();
    void update(float dt);
    std::list<ECE_LaserBlast> &getLasers();
    sf::FloatRect getBounds() const;
    sf::Vector2f getPosition() const;

private:
    sf::Texture texture;
    sf::Sprite sprite;
    std::list<ECE_LaserBlast> lasers;
    float speed;
    bool canShoot;
    float shootCooldown;
    float timeSinceLastShot;
};
