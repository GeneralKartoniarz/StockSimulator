#pragma once
#include "GameState.hpp"

class PlayingState : public GameState {
public:
    PlayingState(Game* game);

    void handleEvent(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void renderImGui() override;
};