#include "BankruptcyState.hpp"
#include "MainMenuState.hpp"
#include "Game.hpp"
#include "imgui.h"
#include <algorithm>
#include <filesystem>
#include <iostream>

BankruptcyState::BankruptcyState(Game* game) 
    : GameState(game)
{
    std::string soundPath = std::filesystem::absolute("assets/gunshot.wav").generic_string();
    if (gunshotBuffer.loadFromFile(soundPath)) {
        gunshotSound.emplace(gunshotBuffer);
        gunshotSound->setVolume(100.0f);
        gunshotSound->play();
    } else {
        std::cerr << "[OSTRZEZENIE] Nie znaleziono pliku dzwieku: " << soundPath << "\n";
    }
}

void BankruptcyState::handleEvent(const sf::Event& event) {
    if (event.is<sf::Event::KeyPressed>() || event.is<sf::Event::MouseButtonPressed>()) {
        game->changeState(std::make_unique<MainMenuState>(game));
    }
}

void BankruptcyState::update(sf::Time deltaTime) {
    displayTimer += deltaTime.asSeconds();
    if (displayTimer >= totalDuration) {
        game->changeState(std::make_unique<MainMenuState>(game));
    }
}

void BankruptcyState::render(sf::RenderWindow& window) {
    window.clear(sf::Color(10, 10, 10));
}

void BankruptcyState::renderImGui() {
    float alphaRatio = 1.0f - (displayTimer / (totalDuration - 0.5f));
    alphaRatio = std::clamp(alphaRatio, 0.0f, 1.0f);

    ImGuiIO& io = ImGui::GetIO();
    ImFont* largeFont = (io.Fonts->Fonts.Size > 1) ? io.Fonts->Fonts[1] : nullptr;

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoInputs |
                             ImGuiWindowFlags_NoBackground;

    ImGui::Begin("BankruptcyWindow", nullptr, flags);

    if (largeFont) ImGui::PushFont(largeFont);

    ImGui::TextColored(ImVec4(0.85f, 0.10f, 0.10f, alphaRatio), "BANKRUT");

    if (largeFont) ImGui::PopFont();

    ImGui::End();

    ImGui::PopStyleVar(2);
}