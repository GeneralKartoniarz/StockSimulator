#include "MainMenuState.hpp"
#include "PlayingState.hpp"
#include "Game.hpp"
#include "imgui.h"

MainMenuState::MainMenuState(Game* game) : GameState(game) {}

void MainMenuState::handleEvent(const sf::Event& event) {}
void MainMenuState::update(sf::Time deltaTime) {}
void MainMenuState::render(sf::RenderWindow& window) {}

void MainMenuState::renderImGui() {
    auto windowSize = game->getWindow().getSize();
    ImGui::SetNextWindowPos(ImVec2(windowSize.x / 2.0f, windowSize.y / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
    
    ImGui::Begin("Menu", nullptr, flags);
    ImGui::Text("=== AUTYSTYCZNY SYMULATOR GIELDY ===");
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    if (ImGui::Button("Start Rozgrywki", ImVec2(300, 50))) {
        game->changeState(std::make_unique<PlayingState>(game));
    }
    ImGui::Spacing();
    
    if (ImGui::Button("Opcje", ImVec2(300, 50))) {}
    
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    if (ImGui::Button("Wyjscie z gry", ImVec2(300, 50))) {
        game->close();
    }
    ImGui::End();
}