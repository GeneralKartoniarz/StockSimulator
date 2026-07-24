#pragma once
#include "GameState.hpp"
#include <SFML/Audio.hpp>
#include <optional>

class BankruptcyState : public GameState {
private:
    sf::SoundBuffer gunshotBuffer;
    std::optional<sf::Sound> gunshotSound;

    float displayTimer = 0.0f;
    const float totalDuration = 4.0f;

public:
    BankruptcyState(Game* game);

    void handleEvent(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void renderImGui() override;
};