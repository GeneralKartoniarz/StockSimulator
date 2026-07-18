#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include "GameState.hpp"

class Game {
private:
    sf::RenderWindow window;
    sf::Clock deltaClock;
    
    std::unique_ptr<GameState> currentState;
    std::unique_ptr<GameState> nextState;

public:
    Game();
    ~Game();

    void changeState(std::unique_ptr<GameState> newState);
    sf::RenderWindow& getWindow();
    void close();
    void run();
};