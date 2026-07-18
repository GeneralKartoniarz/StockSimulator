#include "PlayingState.hpp"
#include "MainMenuState.hpp"
#include "Game.hpp"
#include "imgui.h"

PlayingState::PlayingState(Game* game) : GameState(game) {}

void PlayingState::handleEvent(const sf::Event& event) {}
void PlayingState::update(sf::Time deltaTime) {}
void PlayingState::render(sf::RenderWindow& window) {}

void PlayingState::renderImGui() {
    ImGui::Begin("Rynek");
    ImGui::Text("Statystyki, tabele i wykresy.");
    
    if (ImGui::Button("Zbankrutuj")) {
        game->changeState(std::make_unique<MainMenuState>(game));
    }
    ImGui::End();
}