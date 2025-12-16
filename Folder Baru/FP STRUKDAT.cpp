#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;

struct Rectangle {
    float x, y, width, height;

    Rectangle(float x = 0, float y = 0, float w = 0, float h = 0)
        : x(x), y(y), width(w), height(h) {}

    bool contains(float px, float py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }

    bool intersects(const Rectangle& other) const {
        return !(other.x > x + width || other.x + other.width < x ||
                 other.y > y + height || other.y + other.height < y);
    }
};

struct Particle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;
    float radius;
    int id;

    Particle() : position(0, 0), velocity(0, 0), color(sf::Color::White), radius(5), id(0) {}

    Particle(float x, float y, float vx, float vy, const sf::Color& c, float r, int i)
        : position(x, y), velocity(vx, vy), color(c), radius(r), id(i) {}

    void update(float width, float height) {
        position.x += velocity.x;
        position.y += velocity.y;

        if (position.x - radius < 0) {
            position.x = radius;
            velocity.x = -velocity.x;
        } else if (position.x + radius > width) {
            position.x = width - radius;
            velocity.x = -velocity.x;
        }

        if (position.y - radius < 0) {
            position.y = radius;
            velocity.y = -velocity.y;
        } else if (position.y + radius > height) {
            position.y = height - radius;
            velocity.y = -velocity.y;
        }
    }

    void draw(sf::RenderWindow& window) {
        sf::CircleShape shape(radius);
        shape.setPosition(sf::Vector2f(position.x - radius, position.y - radius));
        shape.setFillColor(color);
        window.draw(shape);
    }
};

class Quadtree {
private:
    static const int MAX_PARTICLES = 4;
    static const int MAX_LEVELS = 8;

    int level;
    std::vector<Particle*> particles;
    Rectangle bounds;
    Quadtree* nodes[4];

    void split() {
        float subWidth = bounds.width / 2.0f;
        float subHeight = bounds.height / 2.0f;
        float x = bounds.x;
        float y = bounds.y;

        nodes[0] = new Quadtree(level + 1, Rectangle(x + subWidth, y, subWidth, subHeight));
        nodes[1] = new Quadtree(level + 1, Rectangle(x, y, subWidth, subHeight));
        nodes[2] = new Quadtree(level + 1, Rectangle(x, y + subHeight, subWidth, subHeight));
        nodes[3] = new Quadtree(level + 1, Rectangle(x + subWidth, y + subHeight, subWidth, subHeight));
    }

    int getIndex(Particle* p) {
        int index = -1;
        float verticalMidpoint = bounds.x + bounds.width / 2.0f;
        float horizontalMidpoint = bounds.y + bounds.height / 2.0f;

        bool topQuadrant = (p->position.y - p->radius < horizontalMidpoint);
        bool bottomQuadrant = (p->position.y + p->radius > horizontalMidpoint);

        if (p->position.x - p->radius < verticalMidpoint &&
            p->position.x + p->radius < verticalMidpoint) {
            if (topQuadrant && !bottomQuadrant) index = 1;
            else if (bottomQuadrant && !topQuadrant) index = 2;
        } else if (p->position.x - p->radius > verticalMidpoint) {
            if (topQuadrant && !bottomQuadrant) index = 0;
            else if (bottomQuadrant && !topQuadrant) index = 3;
        }

        return index;
    }

public:
    Quadtree(int lv, Rectangle b) : level(lv), bounds(b) {
        for (int i = 0; i < 4; i++) nodes[i] = nullptr;
    }

    ~Quadtree() {
        clear();
    }

    void clear() {
        particles.clear();
        for (int i = 0; i < 4; i++) {
            if (nodes[i] != nullptr) {
                delete nodes[i];
                nodes[i] = nullptr;
            }
        }
    }

    void insert(Particle* p) {
        if (nodes[0] != nullptr) {
            int index = getIndex(p);
            if (index != -1) {
                nodes[index]->insert(p);
                return;
            }
        }

        particles.push_back(p);

        if (particles.size() > MAX_PARTICLES && level < MAX_LEVELS) {
            if (nodes[0] == nullptr) split();

            std::vector<Particle*>::iterator it = particles.begin();
            while (it != particles.end()) {
                int index = getIndex(*it);
                if (index != -1) {
                    nodes[index]->insert(*it);
                    it = particles.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    void retrieve(std::vector<Particle*>& returnObjects, Particle* p) {
        int index = getIndex(p);
        if (index != -1 && nodes[0] != nullptr) {
            nodes[index]->retrieve(returnObjects, p);
        }

        returnObjects.insert(returnObjects.end(), particles.begin(), particles.end());

        if (nodes[0] != nullptr && index == -1) {
            for (int i = 0; i < 4; i++) {
                nodes[i]->retrieve(returnObjects, p);
            }
        }
    }

    void draw(sf::RenderWindow& window) {
        sf::RectangleShape rect(sf::Vector2f(bounds.width, bounds.height));
        rect.setPosition(sf::Vector2f(bounds.x, bounds.y));
        rect.setFillColor(sf::Color::Transparent);
        rect.setOutlineColor(sf::Color(50, 50, 50));
        rect.setOutlineThickness(1.0f);
        window.draw(rect);

        if (nodes[0] != nullptr) {
            for (int i = 0; i < 4; i++) {
                nodes[i]->draw(window);
            }
        }
    }
};

sf::Color getRandomColor() {
    int colorChoice = rand() % 10;
    switch(colorChoice) {
        case 0: return sf::Color(100, 150, 255);
        case 1: return sf::Color(150, 100, 255);
        case 2: return sf::Color(255, 100, 150);
        case 3: return sf::Color(100, 255, 150);
        case 4: return sf::Color(150, 255, 200);
        case 5: return sf::Color(200, 150, 255);
        case 6: return sf::Color(255, 200, 100);
        case 7: return sf::Color(100, 200, 150);
        case 8: return sf::Color(255, 150, 100);
        default: return sf::Color(150, 200, 255);
    }
}

float distance(const Particle& a, const Particle& b) {
    float dx = a.position.x - b.position.x;
    float dy = a.position.y - b.position.y;
    return std::sqrt(dx * dx + dy * dy);
}

int checkCollisionsBruteForce(std::vector<Particle>& particles) {
    int collisions = 0;

    for (size_t i = 0; i < particles.size(); i++) {
        for (size_t j = i + 1; j < particles.size(); j++) {
            float dist = distance(particles[i], particles[j]);
            float minDist = particles[i].radius + particles[j].radius;

            if (dist < minDist && dist > 0) {
                collisions++;

                float dx = particles[i].position.x - particles[j].position.x;
                float dy = particles[i].position.y - particles[j].position.y;
                float overlap = minDist - dist;
                dx /= dist;
                dy /= dist;

                particles[i].position.x += dx * (overlap / 2.0f);
                particles[i].position.y += dy * (overlap / 2.0f);
                particles[j].position.x -= dx * (overlap / 2.0f);
                particles[j].position.y -= dy * (overlap / 2.0f);

                float tempVx = particles[i].velocity.x;
                float tempVy = particles[i].velocity.y;
                particles[i].velocity.x = particles[j].velocity.x;
                particles[i].velocity.y = particles[j].velocity.y;
                particles[j].velocity.x = tempVx;
                particles[j].velocity.y = tempVy;
            }
        }
    }

    return collisions;
}

int checkCollisionsQuadtree(std::vector<Particle>& particles, Quadtree& quadtree) {
    int collisions = 0;

    for (size_t i = 0; i < particles.size(); i++) {
        std::vector<Particle*> candidates;
        quadtree.retrieve(candidates, &particles[i]);

        for (size_t j = 0; j < candidates.size(); j++) {
            if (particles[i].id >= candidates[j]->id) continue;

            float dist = distance(particles[i], *candidates[j]);
            float minDist = particles[i].radius + candidates[j]->radius;

            if (dist < minDist && dist > 0) {
                collisions++;

                float dx = particles[i].position.x - candidates[j]->position.x;
                float dy = particles[i].position.y - candidates[j]->position.y;
                float overlap = minDist - dist;
                dx /= dist;
                dy /= dist;

                particles[i].position.x += dx * (overlap / 2.0f);
                particles[i].position.y += dy * (overlap / 2.0f);
                candidates[j]->position.x -= dx * (overlap / 2.0f);
                candidates[j]->position.y -= dy * (overlap / 2.0f);

                float tempVx = particles[i].velocity.x;
                float tempVy = particles[i].velocity.y;
                particles[i].velocity.x = candidates[j]->velocity.x;
                particles[i].velocity.y = candidates[j]->velocity.y;
                candidates[j]->velocity.x = tempVx;
                candidates[j]->velocity.y = tempVy;
            }
        }
    }

    return collisions;
}

int main() {
    srand(static_cast<unsigned>(time(0)));

    int NUM_PARTICLES = 100;

    sf::RenderWindow window;
    window.create(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), 
                  "FP STRUKDAT - Collision Detection");
    window.setFramerateLimit(60);

    std::vector<Particle> particles;
    bool useBruteForce = true;
    bool showQuadtree = false;
    int particleIdCounter = 0;

    // Initialize particles
    for (int i = 0; i < NUM_PARTICLES; i++) {
        float x = static_cast<float>(rand() % WINDOW_WIDTH);
        float y = static_cast<float>(rand() % WINDOW_HEIGHT);
        float vx = (rand() % 400 - 200) / 100.0f;
        float vy = (rand() % 400 - 200) / 100.0f;
        float radius = 8.0f + static_cast<float>(rand() % 5);
        sf::Color color = getRandomColor();

        particles.push_back(Particle(x, y, vx, vy, color, radius, particleIdCounter++));
    }

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
                if (keyPressed->code == sf::Keyboard::Key::Space) {
                    useBruteForce = !useBruteForce;
                }
                if (keyPressed->code == sf::Keyboard::Key::Q) {
                    showQuadtree = !showQuadtree;
                }
                if (keyPressed->code == sf::Keyboard::Key::Up && NUM_PARTICLES < 500) {
                    NUM_PARTICLES += 50;
                    particles.clear();
                    particleIdCounter = 0;
                    for (int i = 0; i < NUM_PARTICLES; i++) {
                        float x = static_cast<float>(rand() % WINDOW_WIDTH);
                        float y = static_cast<float>(rand() % WINDOW_HEIGHT);
                        float vx = (rand() % 400 - 200) / 100.0f;
                        float vy = (rand() % 400 - 200) / 100.0f;
                        float radius = 8.0f + static_cast<float>(rand() % 5);
                        particles.push_back(Particle(x, y, vx, vy, getRandomColor(), radius, particleIdCounter++));
                    }
                }
                if (keyPressed->code == sf::Keyboard::Key::Down && NUM_PARTICLES > 50) {
                    NUM_PARTICLES -= 50;
                    particles.resize(NUM_PARTICLES);
                }
                if (keyPressed->code == sf::Keyboard::Key::R) {
                    particles.clear();
                    particleIdCounter = 0;
                    for (int i = 0; i < NUM_PARTICLES; i++) {
                        float x = static_cast<float>(rand() % WINDOW_WIDTH);
                        float y = static_cast<float>(rand() % WINDOW_HEIGHT);
                        float vx = (rand() % 400 - 200) / 100.0f;
                        float vy = (rand() % 400 - 200) / 100.0f;
                        float radius = 8.0f + static_cast<float>(rand() % 5);
                        particles.push_back(Particle(x, y, vx, vy, getRandomColor(), radius, particleIdCounter++));
                    }
                }
            }
        }

        // Update particles
        for (size_t i = 0; i < particles.size(); i++) {
            particles[i].update(WINDOW_WIDTH, WINDOW_HEIGHT);
        }

        // Check collisions
        if (useBruteForce) {
            checkCollisionsBruteForce(particles);
        } else {
            Quadtree quadtree(0, Rectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT));
            for (size_t i = 0; i < particles.size(); i++) {
                quadtree.insert(&particles[i]);
            }
            checkCollisionsQuadtree(particles, quadtree);

            if (showQuadtree) {
                window.clear(sf::Color::Black);
                quadtree.draw(window);
                for (size_t i = 0; i < particles.size(); i++) {
                    particles[i].draw(window);
                }
                window.display();
                continue;
            }
        }

        // Render
        window.clear(sf::Color::Black);
        for (size_t i = 0; i < particles.size(); i++) {
            particles[i].draw(window);
        }
        window.display();
    }

    return 0;
}