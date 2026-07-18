#include "Game.hpp"
#include "MainMenuState.hpp"
#include "imgui-SFML.h"
#include "implot.h"
#include <optional>

Game::Game() : window(sf::VideoMode({1280, 720}), "Symulator Gieldy") {
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);
    ImPlot::CreateContext();
}

Game::~Game() {
    ImPlot::DestroyContext();
    ImGui::SFML::Shutdown();
}

void Game::changeState(std::unique_ptr<GameState> newState) {
    nextState = std::move(newState);
}

sf::RenderWindow& Game::getWindow() { return window; }
void Game::close() { window.close(); }
void Game::run() {
    currentState = std::make_unique<MainMenuState>(this);

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (currentState) {
                currentState->handleEvent(*event);
            }
        }
        
        sf::Time dt = deltaClock.restart();
        ImGui::SFML::Update(window, dt);

        if (currentState) {
            currentState->update(dt);
        }

        window.clear(sf::Color(20, 20, 20));

        if (currentState) {
            currentState->render(window);
            currentState->renderImGui();
        }

        ImGui::SFML::Render(window);
        window.display();
        if (nextState) {
            currentState = std::move(nextState);
            nextState = nullptr;
        }
    }
}