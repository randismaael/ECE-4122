#ifndef ECE_ENEMY_HPP
#define ECE_ENEMY_HPP

#include <SFML/Graphics.hpp>
#include <list>
#include "ECE_LaserBlast.hpp"

class ECE_Enemy
{
public:
    ECE_Enemy(const std::string& textureFile, float xStart, float yStart);
    void draw(sf::RenderWindow& window);
    void move(float dx, float dy);
    void update(float dt);
    void shoot();
    std::list<ECE_LaserBlast>& getLasers();
    sf::FloatRect getBounds() const;
    sf::Vector2f getPosition() const;
    bool active = true;
private:
    sf::Texture texture;
    sf::Sprite sprite;
    float speed;
    bool canShoot;
    float shootCooldown;
    float timeSinceLastShot;
    std::list<ECE_LaserBlast> lasers;
};

#endif
