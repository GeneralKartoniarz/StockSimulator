#include "PlayingState.hpp"
#include "MainMenuState.hpp"
#include "Game.hpp"
#include "imgui.h"
#include "implot.h"
#include <random>
bool GameTime::update(float dt)
{
    bool hourPassed = false;
    accumulator += dt;
    while (accumulator >= 3.0f)
    {
        accumulator -= 3.0f;
        hour++;
        hourPassed = true;

        if (hour >= 17)
        {
            hour = 9;
            day++;

            if (day > 30)
            {
                day = 1;
                monthInQuarter++;
                if (monthInQuarter > 3)
                {
                    monthInQuarter = 1;
                    quarter++;
                    if (quarter > 4)
                    {
                        quarter = 1;
                        year++;
                    }
                }
            }
        }
    }
    return hourPassed;
}
PlayingState::PlayingState(Game *game) : GameState(game)
{
    generateStartingCompanies();
    generateStartingCommodities();

    timeHistory.push_back(totalSimulatedHours);
    for (auto &c : companies)
        c.priceHistory.push_back(c.currentPrice);
    for (auto &c : commodities)
        c.priceHistory.push_back(c.currentPrice);
}

void PlayingState::generateStartingCompanies()
{
    companies = {
        {0, "MAC", "Macrosoft", "Gigant technologiczny, który zmonopolizował rynek systemów operacyjnych dla lodówek.", 150.50},
        {1, "PPL", "Pear", "Producent absurdalnie drogich telefonów z nadgryzioną gruszką w logo.", 320.00},
        {2, "GGL", "Gargle", "Wyszukiwarka, która wie o Tobie więcej niż Ty sam.", 2500.00},
        {3, "AZN", "Amazonia", "Kiedyś księgarnia, teraz dostarczają paczki dronami i kontrolują 90% chmury.", 3300.00},
        {4, "TSL", "Tusla", "Auta elektryczne wysyłane w kosmos przez ekscentrycznego miliardera.", 750.25},
        {5, "NVD", "Novadia", "Ich karty graficzne są tak drogie, że gracze muszą brać na nie kredyt hipoteczny.", 420.69},
        {6, "MTA", "Metaverse", "Korporacja, która próbuje zamknąć ludzkość w wirtualnych goglach.", 330.10},
        {7, "NFL", "Netflux", "Platforma VOD. Kasują najlepsze seriale po pierwszym sezonie.", 550.00},
        {8, "INT", "Intal", "Producent procesorów. Kiedyś potęga, teraz głównie grzeją pokoje zimą.", 55.40},
        {9, "AMD", "Advanced Micro", "Główny rywal Intala, ratujący rynek przed całkowitym marazmem.", 110.80}};
}

void PlayingState::generateStartingCommodities()
{
    commodities = {
        {0, "Ropa Naftowa (Brent)", "OIL", 75.50, "PLN/bbl"},
        {1, "Krzem (Polikrystaliczny)", "SIL", 15.20, "PLN/kg"},
        {2, "Kobalt", "COB", 35000.00, "PLN/t"},
        {3, "Zloto", "XAU", 1950.00, "PLN/oz"},
        {4, "Uran", "URA", 50.75, "PLN/lb"}};
}

void PlayingState::handleEvent(const sf::Event &event) {}
void PlayingState::render(sf::RenderWindow &window) {}

void PlayingState::update(sf::Time deltaTime)
{
    if (gameTime.update(deltaTime.asSeconds()))
    {
        totalSimulatedHours += 1.0;
        timeHistory.push_back(totalSimulatedHours);

        static std::mt19937 rng(1337);
        std::uniform_real_distribution<double> dist(-0.03, 0.03);

        for (auto &c : companies)
        {
            c.currentPrice += c.currentPrice * dist(rng);
            c.priceHistory.push_back(c.currentPrice);
        }

        for (auto &comm : commodities)
        {
            comm.currentPrice += comm.currentPrice * (dist(rng) * 0.5);
            comm.priceHistory.push_back(comm.currentPrice);
        }
    }
}
void PlayingState::renderClock()
{
    ImGuiIO &io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 10.0f, 10.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

    ImGui::Begin("ZegarGieldowy", nullptr, flags);

    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.8f, 1.0f), "KALENDARZ");
    ImGui::Separator();
    ImGui::Text("Rok: %d | Kwartał: Q%d", gameTime.year, gameTime.quarter);
    ImGui::Text("Miesiąc: %d", gameTime.monthInQuarter);
    ImGui::Text("Dzień: %02d", gameTime.day);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Godzina: %02d:00", gameTime.hour);
    ImGui::ProgressBar(gameTime.accumulator / 3.0f, ImVec2(180.0f, 0.0f), "");

    ImGui::End();
}

void PlayingState::renderImGui()
{

    renderClock();
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::Begin("Centrum Dowodzenia", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Zarządzanie oknami:");
    ImGui::Separator();

    ImGui::Checkbox("Notowania Spółek", &showCompaniesPanel);
    ImGui::SameLine();
    ImGui::Checkbox("Rynek Towarowy", &showCommoditiesPanel);
    ImGui::SameLine();
    ImGui::Checkbox("Szczegóły Analizy", &showDetailsPanel);
    ImGui::SameLine();
    ImGui::Checkbox("Wykresy", &showChartPanel);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Zbankrutuj", ImVec2(-1, 30)))
    {
        game->changeState(std::make_unique<MainMenuState>(game));
    }
    ImGui::End();

    if (showCompaniesPanel)
    {
        ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("Notowania Spółek", &showCompaniesPanel);

        if (ImGui::BeginTable("TabelaGieldowa", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("Ticker");
            ImGui::TableSetupColumn("Cena");
            ImGui::TableSetupColumn("Akcja");
            ImGui::TableHeadersRow();

            for (const auto &company : companies)
            {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%s", company.ticker.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%.2f", company.currentPrice);

                ImGui::TableNextColumn();
                ImGui::PushID(company.id);
                if (ImGui::Button("Analizuj", ImVec2(-1, 0)))
                {
                    selectedCompanyId = company.id;
                    showDetailsPanel = true;
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::End();
    }
    if (showCommoditiesPanel)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 250), ImGuiCond_FirstUseEver);
        ImGui::Begin("Rynek Towarowy", &showCommoditiesPanel);

        if (ImGui::BeginTable("TabelaSurowcow", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("Sym");
            ImGui::TableSetupColumn("Nazwa");
            ImGui::TableSetupColumn("Cena");
            ImGui::TableHeadersRow();

            for (const auto &comm : commodities)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", comm.symbol.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%s", comm.name.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%.2f\n%s", comm.currentPrice, comm.unit.c_str());
            }
            ImGui::EndTable();
        }
        ImGui::End();
    }
    if (showChartPanel)
    {
        ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);
        ImGui::Begin("Analiza Wykresowa", &showChartPanel);
        ImGui::BeginChild("PanelFiltrow", ImVec2(200, 0), true);

        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.8f, 1.0f), "Porównanie Akcji");
        ImGui::Separator();
        for (auto &company : companies)
        {
            ImGui::Checkbox(company.ticker.c_str(), &company.showOnChart);
        }

        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Surowce");
        ImGui::Separator();
        for (auto &comm : commodities)
        {
            ImGui::Checkbox(comm.symbol.c_str(), &comm.showOnChart);
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("ObszarWykresu", ImVec2(0, 0), false);
        if (ImPlot::BeginPlot("Notowania Historyczne", ImVec2(-1, -1)))
        {
            ImPlot::SetupAxis(ImAxis_X1, "Czas (Godziny)", ImPlotAxisFlags_AutoFit);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, totalSimulatedHours + 1.0, ImGuiCond_Always);

            ImPlot::SetupAxis(ImAxis_Y1, "Cena (PLN)", ImPlotAxisFlags_AutoFit);

            for (const auto &company : companies)
            {
                if (company.showOnChart && timeHistory.size() > 0)
                {
                    ImPlot::PlotLine(company.ticker.c_str(), timeHistory.data(), company.priceHistory.data(), (int)timeHistory.size());
                }
            }

            for (const auto &comm : commodities)
            {
                if (comm.showOnChart && timeHistory.size() > 0)
                {
                    ImPlot::PlotLine(comm.symbol.c_str(), timeHistory.data(), comm.priceHistory.data(), (int)timeHistory.size());
                }
            }

            ImPlot::EndPlot();
        }
        ImGui::EndChild();

        ImGui::End();
    }
    if (showDetailsPanel)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_FirstUseEver);
        ImGui::Begin("Szczegóły Analizy", &showDetailsPanel);

        if (selectedCompanyId.has_value())
        {
            const Company *selected = nullptr;
            for (const auto &c : companies)
            {
                if (c.id == selectedCompanyId.value())
                {
                    selected = &c;
                    break;
                }
            }

            if (selected)
            {
                ImGui::Text("Akcje: %s", selected->name.c_str());
                ImGui::Separator();

                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[ Ticker: %s ]", selected->ticker.c_str());
                ImGui::Text("Aktualny kurs: %.2f PLN", selected->currentPrice);

                ImGui::Spacing();
                ImGui::Spacing();

                ImGui::TextWrapped("Profil działalności:\n%s", selected->description.c_str());
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Panel transakcyjny");
                ImGui::Button("KUP", ImVec2(100, 30));
                ImGui::SameLine();
                ImGui::Button("SPRZEDAJ", ImVec2(100, 30));
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Wybierz spółkę z listy, aby wyświetlić raport.");
        }

        ImGui::End();
    }
}