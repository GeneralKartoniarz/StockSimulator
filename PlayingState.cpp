#include "PlayingState.hpp"
#include "MainMenuState.hpp"
#include "Game.hpp"
#include "imgui.h"
#include "implot.h"
#include <random>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "BankruptcyState.hpp"
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
    eventSystem.loadEventsFromJson("assets/events.json");
    timeHistory.push_back(totalSimulatedHours);
    for (auto &c : companies)
        c.priceHistory.push_back(c.currentPrice);
    for (auto &c : commodities)
        c.priceHistory.push_back(c.currentPrice);

    lastRecordedDay = gameTime.day;
}

void PlayingState::generateStartingCompanies()
{
    std::ifstream file("assets/companies.json");
    if (!file.is_open())
    {
        std::cerr << "[BLAD] Nie udalo sie otworzyc pliku assets/companies.json!\n";
        return;
    }

    try
    {
        nlohmann::json j;
        file >> j;
        companies = j.get<std::vector<Company>>();
        std::cout << "[INFO] Wczytano " << companies.size() << " spolek z pliku JSON.\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "[BLAD JSON Companies] " << e.what() << "\n";
    }
}

void PlayingState::generateStartingCommodities()
{
    std::ifstream file("assets/commodities.json");
    if (!file.is_open())
    {
        std::cerr << "[BLAD] Nie udalo sie otworzyc pliku assets/commodities.json!\n";
        return;
    }

    try
    {
        nlohmann::json j;
        file >> j;
        commodities = j.get<std::vector<Commodity>>();
        std::cout << "[INFO] Wczytano " << commodities.size() << " surowcow z pliku JSON.\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "[BLAD JSON Commodities] " << e.what() << "\n";
    }
}

double PlayingState::calculateNetWorth() const
{
    double netWorth = bankBalance;
    for (const auto &[companyId, pos] : portfolio)
    {
        auto it = std::find_if(companies.begin(), companies.end(), [companyId](const Company &c)
                               { return c.id == companyId; });
        if (it != companies.end())
        {
            netWorth += pos.quantity * it->currentPrice;
        }
    }
    return netWorth;
}
bool PlayingState::payBill(int billId)
{
    auto it = std::find_if(inbox.begin(), inbox.end(), [billId](const MailMessage &m)
                           { return m.id == billId; });

    if (it == inbox.end() || it->isResolved)
        return false;

    if (bankBalance >= it->amount)
    {
        bankBalance -= it->amount;
        it->isResolved = true;

        int targetId = it->id;
        std::erase_if(inbox, [targetId](const MailMessage &m)
                      { return m.parentBillId == targetId; });

        return true;
    }
    return false;
}
bool PlayingState::buyShares(int companyId, int quantity)
{
    if (quantity <= 0)
        return false;

    auto it = std::find_if(companies.begin(), companies.end(), [companyId](const Company &c)
                           { return c.id == companyId; });

    if (it == companies.end())
        return false;

    double stockCost = it->currentPrice * quantity;
    double fee = stockCost * suckersFeeRate;
    double totalCost = stockCost + fee;

    if (bankBalance < totalCost)
        return false;

    bankBalance -= totalCost;

    auto &pos = portfolio[companyId];
    pos.companyId = companyId;

    double previousTotalCost = pos.quantity * pos.avgBuyPrice;
    pos.quantity += quantity;
    pos.avgBuyPrice = (previousTotalCost + stockCost) / pos.quantity;

    return true;
}

bool PlayingState::sellShares(int companyId, int quantity)
{
    if (quantity <= 0)
        return false;
    auto itPos = portfolio.find(companyId);
    if (itPos == portfolio.end())
        return false;

    auto &pos = itPos->second;
    if (pos.quantity < quantity)
        return false;

    auto itComp = std::find_if(companies.begin(), companies.end(), [companyId](const Company &c)
                               { return c.id == companyId; });

    if (itComp == companies.end())
        return false;

    double grossPayout = itComp->currentPrice * quantity;
    double fee = grossPayout * suckersFeeRate;
    double netPayout = grossPayout - fee;

    bankBalance += netPayout;
    pos.quantity -= quantity;

    if (pos.quantity <= 0)
    {
        portfolio.erase(companyId);
    }

    return true;
}

void PlayingState::handleEvent(const sf::Event &event) {}
void PlayingState::render(sf::RenderWindow &window) {}

void PlayingState::update(sf::Time deltaTime)
{
    if (gameTime.update(deltaTime.asSeconds()))
    {
        totalSimulatedHours += 0.25;
        timeHistory.push_back(totalSimulatedHours);

        eventSystem.update(0.25);

        if (gameTime.day != lastRecordedDay)
        {
            lastRecordedDay = gameTime.day;
            daysPassedCounter++;

            char timeBuffer[64];
            snprintf(timeBuffer, sizeof(timeBuffer), "R%d M%d D%02d", gameTime.year, gameTime.monthInQuarter, gameTime.day);

            eventSystem.checkAndTriggerRandomEvent(companies, commodities, bankBalance, inbox, nextMailId, timeBuffer);

            if (daysPassedCounter >= 7)
            {
                daysPassedCounter = 0;

                eventSystem.generateWeeklyLivingBill(calculateNetWorth(), inbox, nextMailId, timeBuffer);

                if (!eventSystem.processUnpaidBills(bankBalance, inbox, nextMailId, timeBuffer, game))
                {
                    return;
                }
            }
        }

        static std::mt19937 rng(std::random_device{}());
        std::normal_distribution<double> gauss(0.0, 1.0);

        for (auto &comm : commodities)
        {
            double eventMod = eventSystem.getCommodityTrendModifier(comm.id);
            double shock = gauss(rng) * comm.volatility;
            double totalReturn = comm.baseTrend + eventMod + shock;

            comm.currentPrice = std::max(0.01, comm.currentPrice * (1.0 + totalReturn));
            comm.priceHistory.push_back(comm.currentPrice);

            if (comm.priceHistory.size() > 300)
            {
                comm.priceHistory.erase(comm.priceHistory.begin());
            }
        }

        double oilPrice = commodities[0].currentPrice;
        double silPrice = commodities[1].currentPrice;
        double cobPrice = commodities[2].currentPrice;
        double xauPrice = commodities[3].currentPrice;
        double uraPrice = commodities[4].currentPrice;

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

            double eventMod = eventSystem.getCompanyTrendModifier(c.id, c.sector);

            double marketNoise = gauss(rng) * c.volatility;
            double totalReturn = c.baseTrend + sectorImpact + eventMod + marketNoise;

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

void PlayingState::renderMailPanel()
{
    int unreadCount = 0;
    for (const auto &m : inbox)
    {
        if (!m.isRead)
            unreadCount++;
    }

    std::string windowTitle = "Skrzynka Pocztowa";
    if (unreadCount > 0)
    {
        windowTitle += " (" + std::to_string(unreadCount) + " nowe)###MailWindow";
    }
    else
    {
        windowTitle += "###MailWindow";
    }

    ImVec2 screenCenter(ImGui::GetIO().DisplaySize.x / 2.0f, ImGui::GetIO().DisplaySize.y / 2.0f);
    ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(750, 420), ImGuiCond_FirstUseEver);

    if (ImGui::Begin(windowTitle.c_str(), &showMailPanel))
    {
        ImGui::BeginChild("ListaMaili", ImVec2(260, 0), true);
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.8f, 1.0f), "Odebrane (%zu)", inbox.size());
        ImGui::Separator();

        if (inbox.empty())
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Brak wiadomości.");
        }

        for (auto &mail : inbox)
        {
            ImGui::PushID(mail.id);

            std::string label = mail.sender;
            if (!mail.isRead)
                label = "[NOWA] " + label;
            if (mail.type == MailType::Bill && !mail.isResolved)
                label += " (!)";

            bool isSelected = (selectedMailId.has_value() && selectedMailId.value() == mail.id);
            if (ImGui::Selectable(label.c_str(), isSelected))
            {
                selectedMailId = mail.id;
                mail.isRead = true;
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", mail.subject.c_str());
            }

            ImGui::PopID();
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("TrescMaila", ImVec2(0, 0), true);
        if (selectedMailId.has_value())
        {
            auto it = std::find_if(inbox.begin(), inbox.end(), [this](const MailMessage &m)
                                   { return m.id == selectedMailId.value(); });

            if (it != inbox.end())
            {
                MailMessage &mail = *it;

                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Od: %s", mail.sender.c_str());
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Data: %s", mail.timestamp.c_str());
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Temat: %s", mail.subject.c_str());
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextWrapped("%s", mail.body.c_str());
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (mail.type == MailType::Bill)
                {
                    if (!mail.isResolved)
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Do zapłaty: %.2f PLN", mail.amount);
                        ImGui::Spacing();

                        bool canAfford = (bankBalance >= mail.amount);
                        if (!canAfford)
                            ImGui::BeginDisabled();

                        if (ImGui::Button("ZAPŁAĆ RACHUNEK", ImVec2(180, 32)))
                        {
                            payBill(mail.id);
                        }

                        if (!canAfford)
                        {
                            ImGui::EndDisabled();
                            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Brak wystarczających środków na koncie!");
                        }
                    }
                    else
                    {
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[ V ] RACHUNEK ZAZNACZONY JAKO OPŁACONY");
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();
                if (ImGui::Button("Usuń wiadomość"))
                {
                    int idToDelete = mail.id;
                    selectedMailId = std::nullopt;
                    std::erase_if(inbox, [idToDelete](const MailMessage &m)
                                  { return m.id == idToDelete; });
                }
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Wybierz wiadomość z listy po lewej stronie.");
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void PlayingState::renderPortfolioPanel()
{
    ImGuiIO &io = ImGui::GetIO();
    ImVec2 screenCenter(io.DisplaySize.x / 2.0f, io.DisplaySize.y / 2.0f);
    ImVec2 pivotCenter(0.5f, 0.5f);

    ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, pivotCenter);
    ImGui::SetNextWindowSize(ImVec2(750, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Portfel i Konto Bankowe", &showPortfolioPanel))
    {
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "Konto Bankowe: %.2f PLN", bankBalance);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.1f, 1.0f), "|  Suckers Fee: %.1f%%", suckersFeeRate * 100.0);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "|  Majątek Całkowity: %.2f PLN", calculateNetWorth());

        ImGui::Separator();
        ImGui::Spacing();

        if (portfolio.empty())
        {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Brak akcji w portfelu.");
        }
        else
        {
            if (ImGui::BeginTable("TabelaPortfela", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupColumn("Ticker");
                ImGui::TableSetupColumn("Ilość");
                ImGui::TableSetupColumn("Śr. Cena");
                ImGui::TableSetupColumn("Aktualny Kurs");
                ImGui::TableSetupColumn("Wartość");
                ImGui::TableSetupColumn("Wynik (Po Opłatach)");
                ImGui::TableSetupColumn("Akcja");
                ImGui::TableHeadersRow();

                for (auto it = portfolio.begin(); it != portfolio.end();)
                {
                    auto &[companyId, pos] = *it;

                    const Company *company = nullptr;
                    for (const auto &c : companies)
                    {
                        if (c.id == companyId)
                        {
                            company = &c;
                            break;
                        }
                    }

                    if (!company)
                    {
                        ++it;
                        continue;
                    }

                    ImGui::TableNextRow();

                    double totalValue = pos.quantity * company->currentPrice;
                    double netSellPayout = totalValue * (1.0 - suckersFeeRate);
                    double totalCostBasis = pos.quantity * pos.avgBuyPrice;
                    double profitLoss = netSellPayout - totalCostBasis;
                    double profitLossPercent = (totalCostBasis > 0.0) ? (profitLoss / totalCostBasis) * 100.0 : 0.0;

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", company->ticker.c_str());

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", pos.quantity);

                    ImGui::TableNextColumn();
                    ImGui::Text("%.2f PLN", pos.avgBuyPrice);

                    ImGui::TableNextColumn();
                    ImGui::Text("%.2f PLN", company->currentPrice);

                    ImGui::TableNextColumn();
                    ImGui::Text("%.2f PLN", totalValue);

                    ImGui::TableNextColumn();
                    if (profitLoss >= 0.0)
                    {
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "+%.2f PLN (+%.1f%%)", profitLoss, profitLossPercent);
                    }
                    else
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%.2f PLN (%.1f%%)", profitLoss, profitLossPercent);
                    }

                    ImGui::TableNextColumn();
                    ImGui::PushID(companyId);
                    if (ImGui::Button("Sprzedaj Wszystko"))
                    {
                        sellShares(companyId, pos.quantity);
                        ImGui::PopID();
                        break;
                    }
                    ImGui::PopID();

                    ++it;
                }
                ImGui::EndTable();
            }
        }
    }
    ImGui::End();
}

void PlayingState::renderImGui()
{
    renderClock();

    ImVec2 screenCenter(ImGui::GetIO().DisplaySize.x / 2.0f, ImGui::GetIO().DisplaySize.y / 2.0f);
    ImVec2 pivotCenter(0.5f, 0.5f);

    ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, pivotCenter);
    ImGui::Begin("Centrum Dowodzenia", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Zarządzanie oknami:");
    ImGui::Separator();

    int unreadCount = 0;
    for (const auto &m : inbox)
    {
        if (!m.isRead)
            unreadCount++;
    }
    std::string mailCheckboxLabel = "Skrzynka Pocztowa";
    if (unreadCount > 0)
    {
        mailCheckboxLabel += " (" + std::to_string(unreadCount) + "!)";
    }

    ImGui::Checkbox("Portfel & Bank", &showPortfolioPanel);
    ImGui::SameLine();
    ImGui::Checkbox(mailCheckboxLabel.c_str(), &showMailPanel);
    ImGui::SameLine();
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
        game->changeState(std::make_unique<BankruptcyState>(game)); 
    }
    ImGui::End();

    if (showMailPanel)
    {
        renderMailPanel();
    }

    if (showPortfolioPanel)
    {
        renderPortfolioPanel();
    }

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
        ImGui::SetNextWindowSize(ImVec2(420, 450), ImGuiCond_FirstUseEver);
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
                ImGui::TextWrapped("Profil działalności:\n%s", selected->description.c_str());
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                int ownedShares = 0;
                auto itPos = portfolio.find(selected->id);
                if (itPos != portfolio.end())
                {
                    ownedShares = itPos->second.quantity;
                }
                ImGui::Text("Posiadane akcje: %d szt.", ownedShares);

                ImGui::Spacing();
                ImGui::InputInt("Ilość akcji", &tradeQuantity);
                if (tradeQuantity < 1)
                    tradeQuantity = 1;

                double stockCost = selected->currentPrice * tradeQuantity;
                double fee = stockCost * suckersFeeRate;
                double totalCost = stockCost + fee;

                ImGui::Text("Wartość akcji: %.2f PLN", stockCost);
                ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.1f, 1.0f), "Suckers Fee (%.1f%%): %.2f PLN", suckersFeeRate * 100.0, fee);
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Razem (Kupno): %.2f PLN", totalCost);

                ImGui::Spacing();

                bool canAfford = (bankBalance >= totalCost);
                if (!canAfford)
                    ImGui::BeginDisabled();
                if (ImGui::Button("KUP", ImVec2(120, 30)))
                {
                    buyShares(selected->id, tradeQuantity);
                }
                if (!canAfford)
                {
                    ImGui::EndDisabled();
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Brak środków w banku!");
                }

                ImGui::SameLine();

                bool canSell = (ownedShares >= tradeQuantity);
                if (!canSell)
                    ImGui::BeginDisabled();
                if (ImGui::Button("SPRZEDAJ", ImVec2(120, 30)))
                {
                    sellShares(selected->id, tradeQuantity);
                }
                if (!canSell)
                    ImGui::EndDisabled();
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Wybierz spółkę z listy, aby wyświetlić raport.");
        }

        ImGui::End();
    }
}