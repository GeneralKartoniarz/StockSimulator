#include "PlayingState.hpp"
#include "MainMenuState.hpp"
#include "Game.hpp"
#include "imgui.h"
#include "implot.h"
#include <random>
bool GameTime::update(float dt)
{
    bool tickPassed = false;
    accumulator += dt;
    while (accumulator >= 0.75f)
    {
        accumulator -= 0.75f;
        minute += 15;
        tickPassed = true;

        if (minute >= 60)
        {
            minute = 0;
            hour++;

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
    }
    return tickPassed;
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
        {0, "MAC", "Macrosoft", "Gigant technologiczny, który zmonopolizował rynek systemów operacyjnych dla lodówek.", 150.50, {}, false, Sector::Tech, 0.0005, 0.012, 0.0},
        {1, "PPL", "Pear", "Producent absurdalnie drogich telefonów z nadgryzioną gruszką w logo.", 320.00, {}, false, Sector::Tech, 0.0008, 0.015, 0.0},
        {2, "GGL", "Gargle", "Wyszukiwarka, która wie o Tobie więcej niż Ty sam.", 2500.00, {}, false, Sector::Tech, 0.0006, 0.010, 0.0},
        {3, "AZN", "Amazonia", "Kiedyś księgarnia, teraz dostarczają paczki dronami i kontrolują 90% chmury.", 3300.00, {}, false, Sector::Consumer, 0.0007, 0.014, 0.0},
        {4, "TSL", "Tusla", "Auta elektryczne wysyłane w kosmos przez ekscentrycznego miliardera.", 750.25, {}, false, Sector::Automotive, 0.0002, 0.035, 0.0},
        {5, "NVD", "Novadia", "Ich karty graficzne są tak drogie, że gracze muszą brać na nie kredyt hipoteczny.", 420.69, {}, false, Sector::Tech, 0.0015, 0.025, 0.0},
        {6, "MTA", "Metaverse", "Korporacja, która próbuje zamknąć ludzkość w wirtualnych goglach.", 330.10, {}, false, Sector::Consumer, -0.0002, 0.020, 0.0},
        {7, "NFL", "Netflux", "Platforma VOD. Kasują najlepsze seriale po pierwszym sezonie.", 550.00, {}, false, Sector::Consumer, 0.0001, 0.018, 0.0},
        {8, "INT", "Intal", "Producent procesorów. Kiedyś potęga, teraz głównie grzeją pokoje zimą.", 55.40, {}, false, Sector::Tech, -0.0008, 0.014, 0.0},
        {9, "AMD", "Advanced Micro", "Główny rywal Intala, ratujący rynek przed całkowitym marazmem.", 110.80, {}, false, Sector::Tech, 0.0006, 0.022, 0.0},
        {10, "ORU", "Orzełek SA", "Narodowy monopolista paliwowy. Ceny na ich stacjach rosną, gdy ropa drożeje, i rosną, gdy tanieje.", 85.00, {}, false, Sector::Energy, 0.0003, 0.008, 0.0},
        {11, "IZE", "Izera 2", "Projekt narodowego elektryka. Od lat istnieje tylko w PowerPoint, ale kurs rośnie po każdej kolejnej dotacji rządowej.", 10.00, {}, false, Sector::Automotive, -0.0015, 0.045, 0.0},
        {12, "FSR", "Fasari", "Legendarny producent czerwonych supersamochodów. Ich silniki palą tyle co pracownicy Płazki na przerwie.", 450.00, {}, false, Sector::Automotive, 0.0004, 0.018, 0.0},
        {13, "JEL", "Jelcznik", "Ciężarówki i pojazdy opancerzone. Design nie zmienił się od stanu wojennego, ale rura wydechowa potrafi zaćmić słońce.", 125.30, {}, false, Sector::Automotive, 0.0001, 0.012, 0.0},
        {14, "NUK", "Polski Atom", "Spółka planująca budowę elektrowni jądrowej od 1980 roku. Ich jedynym namacalnym produktem są odprawy dla kolejnych zarządów.", 25.00, {}, false, Sector::Energy, 0.0000, 0.005, 0.0},
        {15, "RLX", "Roleksiorz", "Producent zegarków dla ludzi, którzy chcą pokazać status społeczny, którego nie mają. Kupowane głównie na 80 rat 0%.", 620.00, {}, false, Sector::Luxury, 0.0002, 0.009, 0.0},
        {16, "WND", "WiatrakPol", "Firma stawiająca farmy wiatrowe, które kręcą się tylko wtedy, gdy prezes mocno dmucha w kwartalne sprawozdanie finansowe.", 45.15, {}, false, Sector::Energy, 0.0004, 0.020, 0.0},
        {17, "PZK", "Płazka", "Ogólnopolska sieć sklepów monopolowo-spożywczych. Są na każdym rogu, sprzedając hot-dogi i napoje energetyczne o 22:59.", 180.00, {}, false, Sector::Consumer, 0.0010, 0.011, 0.0},
        {18, "ALG", "AlleDrogo", "Portal aukcyjny, gdzie opłaty serwisowe są wyższe niż cena przedmiotu, a darmowa dostawa wymaga trzech doktoratów z regulaminu.", 95.50, {}, false, Sector::Consumer, 0.0005, 0.015, 0.0},
        {19, "KBB", "KebsCorp", "Ogólnopolska sieć franczyzowa serwująca miszany miszany. Obroty drastycznie rosną w weekendy.", 35.20, {}, false, Sector::Consumer, 0.0008, 0.013, 0.0}};
}

void PlayingState::generateStartingCommodities()
{
    commodities = {
        {0, "Ropa Naftowa (Brent)", "OIL", 75.50, "PLN/bbl", {}, false, 0.0001, 0.007},
        {1, "Krzem (Polikrystaliczny)", "SIL", 15.20, "PLN/kg", {}, false, 0.0002, 0.010},
        {2, "Kobalt", "COB", 35000.00, "PLN/t", {}, false, 0.0001, 0.012},
        {3, "Złoto", "XAU", 1950.00, "PLN/oz", {}, false, 0.00005, 0.004},
        {4, "Uran", "URA", 50.75, "PLN/lb", {}, false, 0.0003, 0.015}};
}

void PlayingState::handleEvent(const sf::Event &event) {}
void PlayingState::render(sf::RenderWindow &window) {}

void PlayingState::update(sf::Time deltaTime)
{
    if (gameTime.update(deltaTime.asSeconds()))
    {
        totalSimulatedHours += 0.25;
        timeHistory.push_back(totalSimulatedHours);

        static std::mt19937 rng(std::random_device{}());
        std::normal_distribution<double> gauss(0.0, 1.0);
        if (gameTime.minute == 0 && gameTime.hour == 9 && gameTime.day == 1 && gameTime.monthInQuarter == 1)
        {
            std::uniform_real_distribution<double> trendShift(-0.0005, 0.0005);
            for (auto &c : companies)
            {
                c.baseTrend += trendShift(rng);
            }
            for (auto &comm : commodities)
            {
                comm.baseTrend += trendShift(rng);
            }
        }
        for (auto &comm : commodities)
        {
            double shock = gauss(rng) * comm.volatility;
            double totalReturn = comm.baseTrend + shock;

            comm.currentPrice = std::max(0.01, comm.currentPrice * (1.0 + totalReturn));
            comm.priceHistory.push_back(comm.currentPrice);

            if (comm.priceHistory.size() > 300)
            {
                comm.priceHistory.erase(comm.priceHistory.begin());
            }
        }

        double oilPrice = commodities[0].currentPrice; // OIL (baza ok. 75.50)
        double silPrice = commodities[1].currentPrice; // SIL (baza ok. 15.20)
        double cobPrice = commodities[2].currentPrice; // COB (baza ok. 35000)
        double xauPrice = commodities[3].currentPrice; // XAU (baza ok. 1950)
        double uraPrice = commodities[4].currentPrice; // URA (baza ok. 50.75)

        // SEKTORY
        for (auto &c : companies)
        {
            double sectorImpact = 0.0;

            switch (c.sector)
            {
            case Sector::Tech:
                if (silPrice > 16.0)
                    sectorImpact -= 0.0005;
                if (cobPrice > 36000.0)
                    sectorImpact -= 0.0003;
                break;

            case Sector::Automotive:
                if (cobPrice > 36000.0)
                    sectorImpact -= 0.0008;
                if (oilPrice > 80.0)
                    sectorImpact -= 0.0004;
                break;

            case Sector::Energy:
                if (oilPrice > 78.0 && c.ticker == "ORU")
                    sectorImpact += 0.0008;
                if (uraPrice > 53.0 && c.ticker == "NUK")
                    sectorImpact += 0.0010;
                break;

            case Sector::Luxury:
                if (xauPrice > 2000.0)
                    sectorImpact += 0.0005;
                break;

            case Sector::Consumer:
                if (oilPrice > 80.0)
                    sectorImpact -= 0.0004;
                break;
            }

            double marketNoise = gauss(rng) * c.volatility;
            double totalReturn = c.baseTrend + sectorImpact + marketNoise;

            c.currentPrice = std::max(0.01, c.currentPrice * (1.0 + totalReturn));
            c.priceHistory.push_back(c.currentPrice);

            if (c.priceHistory.size() > 300)
            {
                c.priceHistory.erase(c.priceHistory.begin());
            }
        }

        if (timeHistory.size() > 300)
        {
            timeHistory.erase(timeHistory.begin());
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
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Godzina: %02d:%02d", gameTime.hour, gameTime.minute);
    ImGui::ProgressBar(gameTime.accumulator / 0.75f, ImVec2(180.0f, 0.0f), "");

    ImGui::End();
}

void PlayingState::renderImGui()
{
    renderClock();

    ImGuiIO &io = ImGui::GetIO();
    ImVec2 screenCenter(io.DisplaySize.x / 2.0f, io.DisplaySize.y / 2.0f);
    ImVec2 pivotCenter(0.5f, 0.5f);

    ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, pivotCenter);
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
        ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, pivotCenter);
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
        ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, pivotCenter);
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
        ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, pivotCenter);
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

        ImGui::Checkbox("Śledź aktualny kurs (Okno 20h)", &autoScrollX);
        ImGui::Spacing();

        if (ImPlot::BeginPlot("Notowania Historyczne", ImVec2(-1, -1)))
        {
            ImPlot::SetupAxis(ImAxis_X1, "Czas (Godziny)", ImPlotAxisFlags_None);
            ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, 0.0, totalSimulatedHours);

            if (autoScrollX)
            {
                double x_max = totalSimulatedHours;
                double x_min = std::max(0.0, x_max - 20.0);
                ImPlot::SetupAxisLimits(ImAxis_X1, x_min, x_max, ImGuiCond_Always);
            }

            ImPlot::SetupAxis(ImAxis_Y1, "Cena (PLN)", ImPlotAxisFlags_AutoFit);

            bool isHovered = ImPlot::IsPlotHovered() && !timeHistory.empty();
            int closestIdx = -1;
            double targetX = 0.0;

            if (isHovered)
            {
                ImPlotPoint mousePos = ImPlot::GetPlotMousePos();
                double min_dist = std::numeric_limits<double>::max();

                for (size_t i = 0; i < timeHistory.size(); ++i)
                {
                    double dist = std::abs(timeHistory[i] - mousePos.x);
                    if (dist < min_dist)
                    {
                        min_dist = dist;
                        closestIdx = static_cast<int>(i);
                    }
                }
                if (closestIdx != -1)
                {
                    targetX = timeHistory[closestIdx];
                }
            }

            for (const auto &company : companies)
            {
                if (company.showOnChart && timeHistory.size() > 0)
                {
                    ImPlot::PlotLine(company.ticker.c_str(), timeHistory.data(), company.priceHistory.data(), (int)timeHistory.size());

                    if (isHovered && closestIdx != -1)
                    {
                        double targetY = company.priceHistory[closestIdx];
                        ImVec4 lineColor = ImPlot::GetLastItemColor();

                        ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, lineColor);
                        ImPlot::PushStyleColor(ImPlotCol_MarkerFill, lineColor);
                        ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Circle);
                        ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, 5.0f);
                        ImPlot::PlotScatter(("##dot_" + company.ticker).c_str(), &targetX, &targetY, 1);
                        ImPlot::PopStyleVar(2);
                        ImPlot::PopStyleColor(2);
                    }
                }
            }

            for (const auto &comm : commodities)
            {
                if (comm.showOnChart && timeHistory.size() > 0)
                {
                    ImPlot::PlotLine(comm.symbol.c_str(), timeHistory.data(), comm.priceHistory.data(), (int)timeHistory.size());

                    if (isHovered && closestIdx != -1)
                    {
                        double targetY = comm.priceHistory[closestIdx];
                        ImVec4 commColor = ImPlot::GetLastItemColor();

                        ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, commColor);
                        ImPlot::PushStyleColor(ImPlotCol_MarkerFill, commColor);
                        ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Circle);
                        ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, 5.0f);

                        ImPlot::PlotScatter(("##dot_" + comm.symbol).c_str(), &targetX, &targetY, 1);

                        ImPlot::PopStyleVar(2);
                        ImPlot::PopStyleColor(2);
                    }
                }
            }

            if (isHovered && closestIdx != -1)
            {
                ImPlot::PlotInfLines("##CrosshairLine", &targetX, 1);
                ImGui::BeginTooltip();
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.8f, 1.0f), "Czas: %.2f h", targetX);
                ImGui::Separator();

                for (const auto &company : companies)
                {
                    if (company.showOnChart)
                    {
                        ImGui::Text("%s: %.2f PLN", company.ticker.c_str(), company.priceHistory[closestIdx]);
                    }
                }
                for (const auto &comm : commodities)
                {
                    if (comm.showOnChart)
                    {
                        ImGui::Text("%s: %.2f %s", comm.symbol.c_str(), comm.priceHistory[closestIdx], comm.unit.c_str());
                    }
                }
                ImGui::EndTooltip();
            }

            ImPlot::EndPlot();
        }
        ImGui::EndChild();
        ImGui::End();
    }

    if (showDetailsPanel)
    {
        ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, pivotCenter);
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