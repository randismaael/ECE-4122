#include <SFML/Graphics.hpp>
#include <list>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <sstream>
#include "ECE_Buzzy.hpp"
#include "ECE_Enemy.hpp"

int main()
{
    sf::RenderWindow window(sf::VideoMode(1152, 1030), "Buzzy Defender");
    window.setFramerateLimit(60);

    enum GameState
    {
        START,
        PLAYING,
        WIN,
        GAME_OVER
    };
    GameState state = START;

    sf::Texture startTexture;
    startTexture.loadFromFile("graphics/Start_Screen.png");
    sf::Sprite startSprite(startTexture);

    sf::Vector2u windowSize = window.getSize();

    // Get original texture size
    sf::Vector2u textureSize = startTexture.getSize();

    // Calculate scale factors
    float scaleX = static_cast<float>(windowSize.x) / textureSize.x;
    float scaleY = static_cast<float>(windowSize.y) / textureSize.y;

    // Apply scaling
    startSprite.setScale(scaleX, scaleY);

    sf::Font font;
    font.loadFromFile("graphics/Pixelletters.ttf");

    std::list<ECE_Enemy> enemies;
    ECE_Buzzy *buzzy = nullptr;

    // Lives system variables
    int lives = 3;
    bool wasHit = false;
    float respawnTimer = 0.0f;
    const float respawnDelay = 2.0f; // 2 seconds of invulnerability after respawn

    std::srand(static_cast<unsigned>(std::time(0)));
    float enemyMoveDownAmount = 10.0f, enemyMoveInterval = 0.6f, enemyMoveTimer = 0;
    bool spacePrev = false;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (state == START && event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter)
            {
                if (buzzy)
                    delete buzzy;
                enemies.clear();

                // Reset lives for new game
                lives = 3;
                wasHit = false;
                respawnTimer = 0.0f;

                // Spawn Buzzy at Top Center
                buzzy = new ECE_Buzzy((1152 - 65) / 2.0f, 10.0f);

                // Spawn enemies in grouped rows (Left, Center, or Right)
                int numRows = 5;
                float rowHeight = 80;
                float startY = 650;
                float spacingX = 70;

                for (int r = 0; r < numRows; ++r)
                {
                    int enemiesInRow = (rand() % 2 == 0) ? 4 : 5;
                    int alignType = rand() % 3; // 0 = left, 1 = center, 2 = right

                    float startX;
                    if (alignType == 0)
                        startX = 100; // Left
                    else if (alignType == 1)
                        startX = (1152 / 2) - (enemiesInRow * spacingX / 2); // Center
                    else
                        startX = 1152 - 100 - (enemiesInRow * spacingX); // Right

                    for (int i = 0; i < enemiesInRow; ++i)
                    {
                        std::string texFile = (rand() % 2 == 0) ? "graphics/bulldog.png" : "graphics/clemson_tigers.png";
                        enemies.emplace_back(texFile, startX + i * spacingX, startY + r * rowHeight);
                    }
                }

                state = PLAYING;
            }

            if ((state == WIN || state == GAME_OVER) && event.type == sf::Event::KeyPressed)
            {
                state = START;
            }
        }

        float dt = 1.f / 60.f;
        bool spaceNow = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);

        if (state == PLAYING)
        {
            // Handle respawn timer
            if (wasHit)
            {
                respawnTimer += dt;
                if (respawnTimer >= respawnDelay)
                {
                    wasHit = false;
                    respawnTimer = 0.0f;
                }
            }

            // Only allow movement and shooting if not in respawn state
            if (!wasHit)
            {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
                    buzzy->move(-buzzy->getBounds().width * dt * 3.2f);
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
                    buzzy->move(buzzy->getBounds().width * dt * 3.2f);
                if (spaceNow && !spacePrev)
                    buzzy->shoot();
            }

            buzzy->update(dt);

            // Enemy update
            for (auto &enemy : enemies)
                enemy.update(dt);

            // Enemy movement downward
            enemyMoveTimer += dt;
            if (enemyMoveTimer > enemyMoveInterval)
            {
                for (auto &enemy : enemies)
                    enemy.move(0, -enemyMoveDownAmount);
                enemyMoveTimer = 0;
            }

            // Random enemy shooting
            if (!enemies.empty() && (rand() % 70 == 1))
                std::next(enemies.begin(), rand() % enemies.size())->shoot();

            // Buzzy lasers hitting enemies
            auto &buzzyLasers = buzzy->getLasers();
            for (auto blit = buzzyLasers.begin(); blit != buzzyLasers.end();)
            {
                bool hit = false;
                for (auto eit = enemies.begin(); eit != enemies.end(); ++eit)
                {
                    if (eit->active && blit->getBounds().intersects(eit->getBounds()))
                    {
                        eit->active = false;
                        hit = true;
                        break;
                    }
                }
                if (hit)
                    blit = buzzyLasers.erase(blit);
                else
                    ++blit;
            }

            // Cleanup inactive enemies
            for (auto eit = enemies.begin(); eit != enemies.end();)
            {
                if (!eit->active)
                {
                    eit->getLasers().clear();
                    eit = enemies.erase(eit);
                }
                else
                {
                    ++eit;
                }
            }

            // Enemy lasers hitting Buzzy (only if not in respawn state)
            if (!wasHit)
            {
                for (auto &enemy : enemies)
                {
                    auto &list = enemy.getLasers();
                    for (auto lit = list.begin(); lit != list.end();)
                    {
                        if (lit->getBounds().intersects(buzzy->getBounds()))
                        {
                            lives--;
                            wasHit = true;
                            respawnTimer = 0.0f;

                            // Remove the laser that hit Buzzy
                            lit = list.erase(lit);

                            if (lives <= 0)
                            {
                                state = GAME_OVER;
                                goto render;
                            }
                            break; // Break out of laser loop since we were hit
                        }
                        else
                        {
                            ++lit;
                        }
                    }
                    if (wasHit)
                        break; // Break out of enemy loop if we were hit
                }
            }

            // Enemy reaches Buzzy zone (only if not in respawn state)
            if (!wasHit)
            {
                for (auto &enemy : enemies)
                {
                    if (enemy.getPosition().y < buzzy->getPosition().y + buzzy->getBounds().height)
                    {
                        lives--;
                        wasHit = true;
                        respawnTimer = 0.0f;

                        if (lives <= 0)
                        {
                            state = GAME_OVER;
                            goto render;
                        }
                        break; // Break out of loop since we were hit
                    }
                }
            }

            if (enemies.empty())
                state = WIN;
        }

    render:
        window.clear(sf::Color(112, 146, 190));
        if (state == START)
        {
            window.draw(startSprite);
        }
        else if (state == PLAYING)
        {
            // Draw Buzzy with flashing effect during respawn
            if (buzzy)
            {
                if (!wasHit || (static_cast<int>(respawnTimer * 10) % 2 == 0))
                {
                    buzzy->draw(window);
                }
            }

            for (auto &enemy : enemies)
                if (enemy.active)
                    enemy.draw(window);

            // Draw lives counter
            sf::Text livesText;
            livesText.setFont(font);
            livesText.setCharacterSize(32);
            livesText.setFillColor(sf::Color::White);
            std::ostringstream oss;
            oss << "Lives: " << lives;
            livesText.setString(oss.str());
            livesText.setPosition(10, 10);
            window.draw(livesText);

            // Draw respawn message if hit
            if (wasHit)
            {
                sf::Text respawnText;
                respawnText.setFont(font);
                respawnText.setCharacterSize(48);
                respawnText.setFillColor(sf::Color::Red);
                respawnText.setString("RESPAWNING...");
                respawnText.setPosition(400, 500);
                window.draw(respawnText);
            }
        }
        else // WIN or GAME_OVER
        {
            sf::Text endText;
            endText.setFont(font);
            endText.setCharacterSize(64);
            endText.setFillColor(sf::Color::White);
            if (state == GAME_OVER)
                endText.setString("GAME OVER\nPress any key");
            else
                endText.setString("YOU WIN!\nPress any key");
            endText.setPosition(200, 400);
            window.clear((state == GAME_OVER) ? sf::Color(220, 50, 50) : sf::Color(50, 180, 50));
            window.draw(endText);
        }
        window.display();
        spacePrev = spaceNow;
    }

    if (buzzy)
        delete buzzy;

    return 0;
}