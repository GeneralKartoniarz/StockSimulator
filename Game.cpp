#include "Game.hpp"
#include "MainMenuState.hpp"
#include "imgui.h"
#include "imgui-SFML.h"
#include "implot.h"
#include <optional>
#include <filesystem>
#include <iostream>
Game::Game() : window(sf::VideoMode({1280, 720}), "Symulator Gieldy") {
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window, false);
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    static const ImWchar ranges[] = {
        0x0020, 0x00FF, 
        0x0100, 0x017F, 
        0
    };
    std::string absolutePath = std::filesystem::absolute("assets/arial.ttf").generic_string();
    io.Fonts->AddFontFromFileTTF(absolutePath.c_str(), 24.0f, nullptr, ranges);
    io.Fonts->AddFontFromFileTTF(absolutePath.c_str(), 90.0f, nullptr, ranges);
    if (!ImGui::SFML::UpdateFontTexture()) {
        std::cerr << "[BLAD] Nie udalo sie zaktualizowac tekstury czcionki w SFML!\n";
    }
}
Game::~Game()
{
    ImPlot::DestroyContext();
    ImGui::SFML::Shutdown();
}

void Game::changeState(std::unique_ptr<GameState> newState)
{
    nextState = std::move(newState);
}

sf::RenderWindow &Game::getWindow() { return window; }
void Game::close() { window.close(); }
void Game::run()
{
    currentState = std::make_unique<MainMenuState>(this);

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            if (currentState)
            {
                currentState->handleEvent(*event);
            }
        }

        sf::Time dt = deltaClock.restart();
        ImGui::SFML::Update(window, dt);

        if (currentState)
        {
            currentState->update(dt);
        }

        window.clear(sf::Color(20, 20, 20));

        if (currentState)
        {
            currentState->render(window);
            currentState->renderImGui();
        }

        ImGui::SFML::Render(window);
        window.display();
        if (nextState)
        {
            currentState = std::move(nextState);
            nextState = nullptr;
        }
    }
}