#pragma once
#include "GameState.hpp"
#include "PlayingState.hpp"
#include <SFML/Audio.hpp>
#include <optional>

class BankruptcyState : public GameState
{
private:
    sf::SoundBuffer soundBuffer;
    std::optional<sf::Sound> sound;

    RunStats finalStats;

public:
    BankruptcyState(Game *game, const RunStats &stats = RunStats{});
    void handleEvent(const sf::Event &event) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow &window) override;
    void renderImGui() override;
};