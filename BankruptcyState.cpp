#include "BankruptcyState.hpp"
#include "MainMenuState.hpp"
#include "Game.hpp"
#include "imgui.h"
#include <filesystem>
#include <iostream>

BankruptcyState::BankruptcyState(Game* game, const RunStats& stats) 
    : GameState(game), finalStats(stats)
{
    if (!finalStats.isVictory)
    {
        std::string soundPath = std::filesystem::absolute("assets/gunshot.wav").generic_string();
        if (soundBuffer.loadFromFile(soundPath)) {
            sound.emplace(soundBuffer);
            sound->setVolume(100.0f);
            sound->play();
        }
    }
}

void BankruptcyState::handleEvent(const sf::Event& event) {
    if (event.is<sf::Event::KeyPressed>() || event.is<sf::Event::MouseButtonPressed>()) {
        game->changeState(std::make_unique<MainMenuState>(game));
    }
}

void BankruptcyState::update(sf::Time deltaTime) {}

void BankruptcyState::render(sf::RenderWindow& window) {
    if (finalStats.isVictory)
        window.clear(sf::Color(10, 30, 10));
    else
        window.clear(sf::Color(20, 5, 5));   
}

void BankruptcyState::renderImGui() {
    ImGuiIO& io = ImGui::GetIO();
    ImFont* largeFont = (io.Fonts->Fonts.Size > 1) ? io.Fonts->Fonts[1] : nullptr;

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(550, 480));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoMove;

    ImGui::Begin("SummaryWindow", nullptr, flags);

    if (largeFont) ImGui::PushFont(largeFont);

    if (finalStats.isVictory)
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "ZWYCIĘSTWO!");
    else
        ImGui::TextColored(ImVec4(0.9f, 0.1f, 0.1f, 1.0f), "BANKRUT!");

    if (largeFont) ImGui::PopFont();

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Powód: %s", finalStats.endReason.c_str());
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "PODSUMOWANIE STATYSTYK RUNU:");
    ImGui::Spacing();

    ImGui::Text("Przeżyte dni: %d", finalStats.daysSurvived);
    ImGui::Text("Maksymalny osiągnięty majątek: %.2f PLN", finalStats.maxNetWorth);
    ImGui::Text("Najlepsza pojedyncza transakcja: +%.2f PLN", finalStats.maxSingleProfit);
    ImGui::Text("Liczba wykonanych transakcji: %d", finalStats.totalTrades);
    ImGui::Text("Łącznie zapłacone prowizje: %.2f PLN", finalStats.totalFeesPaid);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    std::string rank = "Płotką Giełdową";
    if (finalStats.maxNetWorth > 1000000.0) rank = "Prawdziwym Wilkiem z Wall Street";
    else if (finalStats.maxNetWorth > 250000.0) rank = "Inwestorem Wyższej Rangi";
    else if (finalStats.maxNetWorth > 50000.0) rank = "Młodym Wilczkiem";

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Zostałeś: %s", rank.c_str());

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Kliknij dowolny klawisz, aby wrócić do Menu...");

    ImGui::End();
}