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
    loadDarkwebItems();
    eventSystem.loadEventsFromJson("assets/events.json");
    timeHistory.push_back(totalSimulatedHours);
    for (auto &c : companies)
        c.priceHistory.push_back(c.currentPrice);
    for (auto &c : commodities)
        c.priceHistory.push_back(c.currentPrice);

    lastRecordedDay = 0;
    MailMessage welcomeMail;
    welcomeMail.id = nextMailId++;
    welcomeMail.type = MailType::News;
    welcomeMail.sender = "Dom Maklerski";
    welcomeMail.subject = "Witamy na giełdzie! Twój rachunek jest aktywny.";
    welcomeMail.timestamp = "R1 M1 D01";
    welcomeMail.body = "Szanowny Inwestorze,\n\n"
                       "Konto inwestycyjne zostało pomyślnie aktywowane z kapitałem 10 000 PLN.\n"
                       "Steruj prędkością czasu klawiszami [SPACJA, 1, 2, 3] i ustawiaj zlecenia Stop-Loss!\n\nPowodzenia!";
    inbox.push_back(welcomeMail);

    MailMessage startBill;
    startBill.id = nextMailId++;
    startBill.type = MailType::Bill;
    startBill.sender = "Giełda Narodowa w Warszawie";
    startBill.subject = "RACHUNEK: Opłata aktywacyjna lokalu";
    startBill.timestamp = "R1 M1 D01";
    startBill.amount = 350.0;
    startBill.isNew = true;
    startBill.body = "Przesyłamy jednorazową opłatę aktywacyjną za media w Twoim biurze inwestycyjnym.";
    inbox.push_back(startBill);

    selectedMailId = welcomeMail.id;
}

void PlayingState::handleEvent(const sf::Event &event)
{
    if (const auto *keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyPressed->code == sf::Keyboard::Key::Space)
        {
            if (timeSpeedMultiplier > 0.0f)
            {
                savedSpeedMultiplier = timeSpeedMultiplier;
                timeSpeedMultiplier = 0.0f;
            }
            else
            {
                timeSpeedMultiplier = (savedSpeedMultiplier > 0.0f) ? savedSpeedMultiplier : 1.0f;
            }
        }
        else if (keyPressed->code == sf::Keyboard::Key::Num1 || keyPressed->code == sf::Keyboard::Key::Numpad1)
        {
            timeSpeedMultiplier = 1.0f;
        }
        else if (keyPressed->code == sf::Keyboard::Key::Num2 || keyPressed->code == sf::Keyboard::Key::Numpad2)
        {
            timeSpeedMultiplier = 3.0f;
        }
        else if (keyPressed->code == sf::Keyboard::Key::Num3 || keyPressed->code == sf::Keyboard::Key::Numpad3)
        {
            timeSpeedMultiplier = 10.0f;
        }
    }
}

void PlayingState::addPendingOrder(int companyId, OrderType type, int quantity, double targetPrice)
{
    if (quantity <= 0 || targetPrice <= 0.0)
        return;

    PendingOrder order;
    order.id = nextOrderId++;
    order.companyId = companyId;
    order.type = type;
    order.quantity = quantity;
    order.targetPrice = targetPrice;

    pendingOrders.push_back(order);
}

void PlayingState::cancelPendingOrder(int orderId)
{
    std::erase_if(pendingOrders, [orderId](const PendingOrder &o)
                  { return o.id == orderId; });
}

void PlayingState::processPendingOrders()
{
    for (auto it = pendingOrders.begin(); it != pendingOrders.end();)
    {
        auto compIt = std::find_if(companies.begin(), companies.end(), [it](const Company &c)
                                   { return c.id == it->companyId; });

        if (compIt == companies.end())
        {
            it = pendingOrders.erase(it);
            continue;
        }

        bool triggered = false;
        bool success = false;

        if (it->type == OrderType::LimitBuy && compIt->currentPrice <= it->targetPrice)
        {
            triggered = true;
            success = buyShares(it->companyId, it->quantity);
        }
        else if (it->type == OrderType::LimitSell && compIt->currentPrice >= it->targetPrice)
        {
            triggered = true;
            success = sellShares(it->companyId, it->quantity);
        }
        else if (it->type == OrderType::StopLoss && compIt->currentPrice <= it->targetPrice)
        {
            triggered = true;
            success = sellShares(it->companyId, it->quantity);
        }

        if (triggered)
        {
            char timeBuf[64];
            snprintf(timeBuf, sizeof(timeBuf), "R%d M%d D%02d", gameTime.year, gameTime.monthInQuarter, gameTime.day);
            it = pendingOrders.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
void PlayingState::update(sf::Time deltaTime)
{
    float dt = deltaTime.asSeconds() * timeSpeedMultiplier;

    if (gameTime.update(dt))
    {
        totalSimulatedHours += 0.25;
        timeHistory.push_back(totalSimulatedHours);

        eventSystem.update(0.25);

        if (gameTime.day != lastRecordedDay)
        {
            lastRecordedDay = gameTime.day;
            daysPassedCounter++;
            stats.daysSurvived++;

            char timeBuffer[64];
            snprintf(timeBuffer, sizeof(timeBuffer), "R%d M%d D%02d", gameTime.year, gameTime.monthInQuarter, gameTime.day);

            if (gameTime.day == 1)
            {
                bankSystem.updateMacroeconomy(inbox, nextMailId, timeBuffer);
            }

            eventSystem.processPendingEvents(inbox, nextMailId, timeBuffer);
            eventSystem.checkAndTriggerRandomEvent(companies, commodities, bankBalance, inbox, nextMailId, timeBuffer);

            if (daysPassedCounter >= 7)
            {
                daysPassedCounter = 0;

                bankSystem.processWeeklyInterest(bankBalance, inbox, nextMailId, timeBuffer);
                eventSystem.generateWeeklyLivingBill(calculateNetWorth(), inbox, nextMailId, timeBuffer);

                if (!eventSystem.processUnpaidBills(bankBalance, inbox, nextMailId, timeBuffer, game))
                {
                    return;
                }
            }

            double currentNetWorth = calculateNetWorth();
            if (currentNetWorth > stats.maxNetWorth)
            {
                stats.maxNetWorth = currentNetWorth;
            }

            if (!isFreeplay && currentNetWorth >= 1000000.0 && !showVictoryModal)
            {
                showVictoryModal = true;
            }

            if (!isFreeplay && gameTime.day == 30 && gameTime.monthInQuarter == 3)
            {
                double requiredQuota = getQuarterlyQuota(gameTime.year, gameTime.quarter);
                if (currentNetWorth < requiredQuota)
                {
                    stats.isVictory = false;
                    stats.endReason = "Niewykonanie celu kwartalnego (" + std::to_string((int)requiredQuota) + " PLN)";
                    game->changeState(std::make_unique<BankruptcyState>(game, stats));
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

            if (comm.priceHistory.size() > MAX_HISTORY_SIZE)
            {
                comm.priceHistory.erase(comm.priceHistory.begin());
            }
        }

        double oilPrice = commodities[0].currentPrice;
        double silPrice = commodities[1].currentPrice;
        double cobPrice = commodities[2].currentPrice;
        double xauPrice = commodities[3].currentPrice;
        double uraPrice = commodities[4].currentPrice;

        double interestRateImpact = 0.0;
        if (bankSystem.getBaseInterestRate() > 0.07)
        {
            interestRateImpact = -0.0003 * (bankSystem.getBaseInterestRate() / 0.07);
        }

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

            double totalReturn = c.baseTrend + sectorImpact + eventMod + interestRateImpact + marketNoise;

            c.currentPrice = std::max(0.01, c.currentPrice * (1.0 + totalReturn));
            c.priceHistory.push_back(c.currentPrice);

            if (c.priceHistory.size() > MAX_HISTORY_SIZE)
            {
                c.priceHistory.erase(c.priceHistory.begin());
            }
        }

        processPendingOrders();

        if (timeHistory.size() > MAX_HISTORY_SIZE)
        {
            timeHistory.erase(timeHistory.begin());
        }
        if (!timeHistory.empty())
        {
            double oldestVisibleTime = timeHistory.front();
            std::erase_if(tradeMarkers, [oldestVisibleTime](const TradeMarker &marker)
                          { return marker.timeX < oldestVisibleTime; });
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
    ImGui::Text("Miesiąc: %d | Dzień: %02d", gameTime.monthInQuarter, gameTime.day);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Godzina: %02d:%02d", gameTime.hour, gameTime.minute);

    ImGui::ProgressBar(gameTime.accumulator / 0.75f, ImVec2(280.0f, 0.0f), "");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "KONTROLA CZASU");
    ImGui::Spacing();

    bool isPaused = (timeSpeedMultiplier == 0.0f);
    bool is1x = (timeSpeedMultiplier == 1.0f);
    bool is3x = (timeSpeedMultiplier == 3.0f);
    bool is10x = (timeSpeedMultiplier == 10.0f);

    if (isPaused)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("||", ImVec2(48, 34)))
    {
        if (timeSpeedMultiplier > 0.0f)
        {
            savedSpeedMultiplier = timeSpeedMultiplier;
            timeSpeedMultiplier = 0.0f;
        }
        else
            timeSpeedMultiplier = (savedSpeedMultiplier > 0.0f) ? savedSpeedMultiplier : 1.0f;
    }
    if (isPaused)
        ImGui::PopStyleColor();

    ImGui::SameLine();
    if (is1x)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    if (ImGui::Button("> 1x", ImVec2(60, 34)))
        timeSpeedMultiplier = 1.0f;
    if (is1x)
        ImGui::PopStyleColor();

    ImGui::SameLine();
    if (is3x)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    if (ImGui::Button(">> 3x", ImVec2(70, 34)))
        timeSpeedMultiplier = 3.0f;
    if (is3x)
        ImGui::PopStyleColor();

    ImGui::SameLine();
    if (is10x)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    if (ImGui::Button(">>> 10x", ImVec2(88, 34)))
        timeSpeedMultiplier = 10.0f;
    if (is10x)
        ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    if (!isFreeplay)
    {
        double target = getQuarterlyQuota(gameTime.year, gameTime.quarter);
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "CEL Q%d: %.0f PLN", gameTime.quarter, target);
        ImGui::ProgressBar(calculateNetWorth() / target, ImVec2(280.0f, 0.0f));
    }
    else
    {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[ TRYB FREEPLAY ]");
    }
    ImGui::End();
}

void PlayingState::renderPortfolioPanel()
{
    ImGuiIO &io = ImGui::GetIO();
    ImVec2 screenCenter(io.DisplaySize.x / 2.0f, io.DisplaySize.y / 2.0f);
    ImVec2 pivotCenter(0.5f, 0.5f);

    ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, pivotCenter);
    ImGui::SetNextWindowSize(ImVec2(800, 480), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Portfel & Konto Bankowe", &showPortfolioPanel))
    {
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "Konto Bankowe: %.2f PLN", bankBalance);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.1f, 1.0f), "|  Suckers Fee: %.1f%%", suckersFeeRate * 100.0);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "|  Majątek Całkowity: %.2f PLN", calculateNetWorth());

        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::TreeNodeEx("Posiadane Akcje", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (portfolio.empty())
            {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Brak akcji w portfelu.");
            }
            else
            {
                if (ImGui::BeginTable("TabelaPortfela", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 180)))
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
                            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "+%.2f PLN (+%.1f%%)", profitLoss, profitLossPercent);
                        else
                            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%.2f PLN (%.1f%%)", profitLoss, profitLossPercent);

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
            ImGui::TreePop();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::TreeNodeEx("Aktywne Zlecenia Oczekujące", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (pendingOrders.empty())
            {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Brak aktywnych zleceń oczekujących.");
            }
            else
            {
                if (ImGui::BeginTable("TabelaZlecen", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                {
                    ImGui::TableSetupColumn("Typ Zlecenia");
                    ImGui::TableSetupColumn("Spółka");
                    ImGui::TableSetupColumn("Ilość");
                    ImGui::TableSetupColumn("Cena Docelowa");
                    ImGui::TableSetupColumn("Aktualny Kurs");
                    ImGui::TableSetupColumn("Akcja");
                    ImGui::TableHeadersRow();

                    for (const auto &ord : pendingOrders)
                    {
                        const Company *comp = nullptr;
                        for (const auto &c : companies)
                        {
                            if (c.id == ord.companyId)
                            {
                                comp = &c;
                                break;
                            }
                        }

                        ImGui::TableNextRow();

                        ImGui::TableNextColumn();
                        if (ord.type == OrderType::LimitBuy)
                            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "LIMIT BUY");
                        else if (ord.type == OrderType::LimitSell)
                            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "LIMIT SELL");
                        else
                            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "STOP LOSS");

                        ImGui::TableNextColumn();
                        ImGui::Text("%s", comp ? comp->ticker.c_str() : "???");
                        ImGui::TableNextColumn();
                        ImGui::Text("%d szt.", ord.quantity);
                        ImGui::TableNextColumn();
                        ImGui::Text("%.2f PLN", ord.targetPrice);
                        ImGui::TableNextColumn();
                        ImGui::Text("%.2f PLN", comp ? comp->currentPrice : 0.0);

                        ImGui::TableNextColumn();
                        ImGui::PushID(ord.id);
                        if (ImGui::Button("Anuluj"))
                        {
                            cancelPendingOrder(ord.id);
                            ImGui::PopID();
                            break;
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

void PlayingState::generateStartingCompanies()
{
    std::ifstream file("assets/companies.json");
    if (!file.is_open())
        return;
    try
    {
        nlohmann::json j;
        file >> j;
        companies = j.get<std::vector<Company>>();
    }
    catch (...)
    {
    }
}

void PlayingState::generateStartingCommodities()
{
    std::ifstream file("assets/commodities.json");
    if (!file.is_open())
        return;
    try
    {
        nlohmann::json j;
        file >> j;
        commodities = j.get<std::vector<Commodity>>();
    }
    catch (...)
    {
    }
}

void PlayingState::loadDarkwebItems()
{
    std::ifstream file("assets/darkweb_items.json");
    if (!file.is_open())
        return;
    try
    {
        nlohmann::json j;
        file >> j;
        darkwebItems = j.get<std::vector<DarkwebItem>>();
    }
    catch (...)
    {
    }
}

bool PlayingState::buyDarkwebItem(int itemId)
{
    auto it = std::find_if(darkwebItems.begin(), darkwebItems.end(), [itemId](const DarkwebItem &item)
                           { return item.id == itemId; });

    if (it == darkwebItems.end() || it->isPurchased)
        return false;

    if (bankBalance >= it->price)
    {
        bankBalance -= it->price;
        it->isPurchased = true;

        char timeBuffer[64];
        snprintf(timeBuffer, sizeof(timeBuffer), "R%d M%d D%02d", gameTime.year, gameTime.monthInQuarter, gameTime.day);

        MailMessage confirmMail;
        confirmMail.id = nextMailId++;
        confirmMail.type = MailType::News;
        confirmMail.sender = "Darkweb Vendor";
        confirmMail.subject = "POTWIERDZENIE ZAKUPU: " + it->name;
        confirmMail.timestamp = timeBuffer;
        confirmMail.body = "Transakcja zakończona sukcesem.\nZakupiono przedmiot: " + it->name + ".";
        inbox.push_back(confirmMail);

        return true;
    }
    return false;
}

double PlayingState::calculateNetWorth() const
{
    double netWorth = bankBalance;
    for (const auto &[companyId, pos] : portfolio)
    {
        auto it = std::find_if(companies.begin(), companies.end(), [companyId](const Company &c)
                               { return c.id == companyId; });
        if (it != companies.end())
            netWorth += pos.quantity * it->currentPrice;
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
    stats.totalFeesPaid += fee;
    stats.totalTrades++;

    auto &pos = portfolio[companyId];
    pos.companyId = companyId;
    double previousTotalCost = pos.quantity * pos.avgBuyPrice;
    pos.quantity += quantity;
    pos.avgBuyPrice = (previousTotalCost + stockCost) / pos.quantity;

    auto itMarker = std::find_if(tradeMarkers.rbegin(), tradeMarkers.rend(),
                                 [companyId, this](const TradeMarker &m)
                                 {
                                     return m.companyId == companyId && m.isBuy && std::abs(m.timeX - totalSimulatedHours) < 0.1;
                                 });

    if (itMarker != tradeMarkers.rend())
    {
        double totalOldCost = itMarker->quantity * itMarker->buyPrice;
        double totalNewCost = quantity * it->currentPrice;
        itMarker->quantity += quantity;
        itMarker->buyPrice = (totalOldCost + totalNewCost) / itMarker->quantity;
        itMarker->priceY = itMarker->buyPrice;
    }
    else
    {
        char dateBuf[64];
        snprintf(dateBuf, sizeof(dateBuf), "R%d M%d D%02d", gameTime.year, gameTime.monthInQuarter, gameTime.day);

        TradeMarker marker;
        marker.companyId = companyId;
        marker.timeX = totalSimulatedHours;
        marker.priceY = it->currentPrice;
        marker.quantity = quantity;
        marker.buyPrice = it->currentPrice;
        marker.dateStr = dateBuf;
        marker.isBuy = true;
        tradeMarkers.push_back(marker);
    }
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

    double profitOnThisTrade = (itComp->currentPrice - pos.avgBuyPrice) * quantity - fee;
    if (profitOnThisTrade > stats.maxSingleProfit)
        stats.maxSingleProfit = profitOnThisTrade;

    stats.totalFeesPaid += fee;
    stats.totalTrades++;
    bankBalance += netPayout;
    pos.quantity -= quantity;

    auto itMarker = std::find_if(tradeMarkers.rbegin(), tradeMarkers.rend(),
                                 [companyId, this](const TradeMarker &m)
                                 {
                                     return m.companyId == companyId && !m.isBuy && std::abs(m.timeX - totalSimulatedHours) < 0.1;
                                 });

    if (itMarker != tradeMarkers.rend())
    {
        double totalOldRevenue = itMarker->quantity * itMarker->buyPrice;
        double totalNewRevenue = quantity * itComp->currentPrice;
        itMarker->quantity += quantity;
        itMarker->buyPrice = (totalOldRevenue + totalNewRevenue) / itMarker->quantity;
        itMarker->priceY = itMarker->buyPrice;
        itMarker->profitLoss += profitOnThisTrade;
    }
    else
    {
        char dateBuf[64];
        snprintf(dateBuf, sizeof(dateBuf), "R%d M%d D%02d", gameTime.year, gameTime.monthInQuarter, gameTime.day);

        TradeMarker marker;
        marker.companyId = companyId;
        marker.timeX = totalSimulatedHours;
        marker.priceY = itComp->currentPrice;
        marker.quantity = quantity;
        marker.buyPrice = itComp->currentPrice;
        marker.dateStr = dateBuf;
        marker.isBuy = false;
        marker.profitLoss = profitOnThisTrade;
        tradeMarkers.push_back(marker);
    }

    if (pos.quantity <= 0)
        portfolio.erase(companyId);
    return true;
}

void PlayingState::render(sf::RenderWindow &window) {}

void PlayingState::renderVictoryModal()
{
    if (!showVictoryModal)
        return;

    ImGui::OpenPopup("ZWYCIĘSTWO!");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(450, 250));

    if (ImGui::BeginPopupModal("ZWYCIĘSTWO!", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "GRATULACJE! ZGROMADZIŁEŚ 1 000 000 PLN!");
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextWrapped("Stałeś się legendą Wall Street!");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("PRZEJDŹ DO FREEPLAY", ImVec2(180, 35)))
        {
            isFreeplay = true;
            showVictoryModal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("ZAKOŃCZ", ImVec2(180, 35)))
        {
            stats.isVictory = true;
            stats.endReason = "Osiągnięto status Milionera!";
            game->changeState(std::make_unique<BankruptcyState>(game, stats));
        }
        ImGui::EndPopup();
    }
}

void PlayingState::renderBrowserPanel()
{
    int unreadMails = 0;
    for (const auto &m : inbox)
    {
        if (!m.isRead)
            unreadMails++;
    }

    ImVec2 screenCenter(ImGui::GetIO().DisplaySize.x / 2.0f, ImGui::GetIO().DisplaySize.y / 2.0f);
    ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(820, 520), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Przeglądarka Firewolf###BrowserWindow", &showBrowserPanel))
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.14f, 1.0f));
        ImGui::BeginChild("BrowserHeader", ImVec2(0, 36), true, ImGuiWindowFlags_NoScrollbar);

        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.0f, 1.0f), "FIREWOLF v3.1");
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        std::string currentUrl = "https://mail.firewolf.net/inbox";
        if (activeBrowserTab == 1)
            currentUrl = "https://online.pkobp.pl/dashboard";
        else if (activeBrowserTab == 2)
            currentUrl = "http://darknet666onion.onion/market";

        ImGui::SetNextItemWidth(450.0f);
        ImGui::InputText("##URL", currentUrl.data(), currentUrl.size(), ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        if (ImGui::Button("Odśwież"))
        {
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Spacing();

        if (ImGui::BeginTabBar("FirewolfTabs", ImGuiTabBarFlags_None))
        {
            std::string mailTabTitle = "Poczta";
            if (unreadMails > 0)
                mailTabTitle += " (" + std::to_string(unreadMails) + ")###MailTab";
            else
                mailTabTitle += "###MailTab";

            if (ImGui::BeginTabItem(mailTabTitle.c_str()))
            {
                activeBrowserTab = 0;
                renderMailTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Bank Centralny###BankTab"))
            {
                activeBrowserTab = 1;
                renderBankTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Darkweb Market###DarkwebTab"))
            {
                activeBrowserTab = 2;
                renderDarkwebTab();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void PlayingState::renderMailTab()
{
    ImGui::BeginChild("ListaMaili", ImVec2(280, 0), true);
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.8f, 1.0f), "Odebrane (%zu)", inbox.size());
    ImGui::Separator();

    if (inbox.empty())
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Brak wiadomości.");

    for (auto &mail : inbox)
    {
        ImGui::PushID(mail.id);
        std::string label = mail.sender;
        if (!mail.isRead)
            label = "[NOWA] " + label;
        if (mail.type == MailType::Bill && !mail.isResolved)
            label += " (!)";
        if (mail.type == MailType::TipOffer && !mail.isResolved)
            label += " ($)";

        bool isSelected = (selectedMailId.has_value() && selectedMailId.value() == mail.id);
        if (ImGui::Selectable(label.c_str(), isSelected))
        {
            selectedMailId = mail.id;
            mail.isRead = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", mail.subject.c_str());
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
                        payBill(mail.id);
                    if (!canAfford)
                    {
                        ImGui::EndDisabled();
                        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Brak wystarczających środków na koncie!");
                    }
                }
                else
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[ V ] RACHUNEK ZAZNACZONY JAKO OPŁACONY");
            }
            else if (mail.type == MailType::TipOffer)
            {
                if (!mail.isResolved)
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Koszt informacji: %.2f PLN", mail.amount);
                    ImGui::Spacing();
                    bool canAfford = (bankBalance >= mail.amount);
                    if (!canAfford)
                        ImGui::BeginDisabled();

                    char timeBuffer[64];
                    snprintf(timeBuffer, sizeof(timeBuffer), "R%d M%d D%02d", gameTime.year, gameTime.monthInQuarter, gameTime.day);

                    if (ImGui::Button("KUP INFORMACJĘ", ImVec2(200, 32)))
                        eventSystem.buyTip(mail.id, bankBalance, inbox, nextMailId, timeBuffer, companies);

                    if (!canAfford)
                    {
                        ImGui::EndDisabled();
                        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Brak środków na koncie!");
                    }
                }
                else
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[ V ] INFORMACJA ZAKUPIONA");
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
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Wybierz wiadomość z listy po lewej stronie.");
    ImGui::EndChild();
}
void PlayingState::renderPieChartsWindow()
{
    if (!showPieChartsWindow)
        return;

    ImVec2 screenCenter(ImGui::GetIO().DisplaySize.x / 2.0f, ImGui::GetIO().DisplaySize.y / 2.0f);
    ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(780, 440), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Analiza Majątku i Dywersyfikacji###PieChartsWin", &showPieChartsWindow))
    {
        ImGui::Columns(2, "PieColumns", true);

        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.8f, 1.0f), "STRUKTURA MAJĄTKU & DŁUGU");
        ImGui::Separator();
        ImGui::Spacing();

        double stocksValue = 0.0;
        for (const auto &[compIndex, pos] : portfolio)
        {
            auto compIt = std::find_if(companies.begin(), companies.end(), [compIndex](const Company &c)
                                       { return c.id == compIndex; });
            if (compIt != companies.end())
            {
                stocksValue += pos.quantity * compIt->currentPrice;
            }
        }
        double cashValue = bankBalance;
        double debtValue = bankSystem.getPlayerDebt();
        double totalAssets = cashValue + stocksValue + debtValue;

        std::vector<std::string> assetLabelStrs;
        std::vector<const char *> assetLabels;
        std::vector<double> assetValues;

        if (totalAssets > 0.0)
        {
            if (cashValue > 0)
            {
                assetLabelStrs.push_back("Gotówka (" + std::to_string((int)cashValue) + " PLN)");
                assetValues.push_back((cashValue / totalAssets) * 100.0);
            }
            if (stocksValue > 0)
            {
                assetLabelStrs.push_back("Akcje (" + std::to_string((int)stocksValue) + " PLN)");
                assetValues.push_back((stocksValue / totalAssets) * 100.0);
            }
            if (debtValue > 0)
            {
                assetLabelStrs.push_back("Kredyt (" + std::to_string((int)debtValue) + " PLN)");
                assetValues.push_back((debtValue / totalAssets) * 100.0);
            }

            for (const auto &str : assetLabelStrs)
            {
                assetLabels.push_back(str.c_str());
            }
        }

        if (!assetValues.empty())
        {
            if (ImPlot::BeginPlot("##AssetsPiePlot", ImVec2(-1, 300), ImPlotFlags_Equal | ImPlotFlags_NoMouseText))
            {
                ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
                ImPlot::PlotPieChart(assetLabels.data(), assetValues.data(), static_cast<int>(assetValues.size()), 0.5, 0.5, 0.4, "%.1f%%");
                ImPlot::EndPlot();
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Brak danych do wyświetlenia.");
        }

        ImGui::NextColumn();

        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "DYWERSIFIKACJA SEKTOROWA");
        ImGui::Separator();
        ImGui::Spacing();

        std::map<Sector, double> sectorValues;
        double totalSectorValue = 0.0;

        for (const auto &[compIndex, pos] : portfolio)
        {
            auto compIt = std::find_if(companies.begin(), companies.end(), [compIndex](const Company &c)
                                       { return c.id == compIndex; });
            if (compIt != companies.end() && pos.quantity > 0)
            {
                double val = pos.quantity * compIt->currentPrice;
                sectorValues[compIt->sector] += val;
                totalSectorValue += val;
            }
        }

        std::vector<std::string> sectorLabelStrs;
        std::vector<const char *> sectorLabels;
        std::vector<double> sectorValVec;

        auto getSectorName = [](Sector s) -> std::string
        {
            switch (s)
            {
            case Sector::Tech:
                return "Tech";
            case Sector::Automotive:
                return "Automotive";
            case Sector::Energy:
                return "Energy";
            case Sector::Luxury:
                return "Luxury";
            case Sector::Consumer:
                return "Consumer";
            default:
                return "Inne";
            }
        };

        if (totalSectorValue > 0.0)
        {
            for (const auto &[sec, val] : sectorValues)
            {
                if (val > 0)
                {
                    sectorLabelStrs.push_back(getSectorName(sec) + " (" + std::to_string((int)val) + " PLN)");
                    sectorValVec.push_back((val / totalSectorValue) * 100.0);
                }
            }

            for (const auto &name : sectorLabelStrs)
            {
                sectorLabels.push_back(name.c_str());
            }
        }

        if (!sectorValVec.empty())
        {
            if (ImPlot::BeginPlot("##SectorPiePlot", ImVec2(-1, 300), ImPlotFlags_Equal | ImPlotFlags_NoMouseText))
            {
                ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
                ImPlot::PlotPieChart(sectorLabels.data(), sectorValVec.data(), static_cast<int>(sectorValVec.size()), 0.5, 0.5, 0.4, "%.1f%%");
                ImPlot::EndPlot();
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Brak posiadanych akcji w portfelu.");
        }

        ImGui::Columns(1);
    }
    ImGui::End();
}
void PlayingState::renderBankTab()
{
    ImGui::BeginChild("BankContent", ImVec2(0, 0), true);

    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.8f, 1.0f), "PKO BANK CENTRALNY S.A. - SYSTEM TRANSAKCYJNY");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Checkbox("Otwórz Wykresy Kołowe Majątku & Dywersyfikacji", &showPieChartsWindow);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Columns(2, "BankColumns", true);

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "WSKAŹNIKI MAKROEKONOMICZNE");
    ImGui::Separator();
    ImGui::Text("Inflacja roczna: %.1f%%", bankSystem.getInflationRate() * 100.0);
    ImGui::Text("Stopa referencyjna NBP: %.1f%%", bankSystem.getBaseInterestRate() * 100.0);
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Oprocentowanie kredytu: %.1f%%", bankSystem.getLoanInterestRate() * 100.0);

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "OCENA KREDYTOWA I MAJĄTEK");
    ImGui::Separator();
    ImGui::Text("Majątek całkowity: %.2f PLN", calculateNetWorth());
    ImGui::Text("Mnożnik oceny kredytowej: %.1fx", bankSystem.getCreditScoreMultiplier());

    ImGui::NextColumn();

    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "ZARZĄDZANIE KREDYTEM");
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Aktualne zadłużenie: %.2f PLN", bankSystem.getPlayerDebt());

    double maxLoan = bankSystem.calculateMaxLoan(calculateNetWorth());
    ImGui::Text("Dostępna zdolność kredytowa: %.2f PLN", maxLoan);
    ImGui::Text("Tygodniowa rata odsetkowa: %.2f PLN", bankSystem.calculateWeeklyInterest());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::InputDouble("Kwota (PLN)", &loanAmountInput, 100.0, 1000.0, "%.2f");
    if (loanAmountInput < 100.0)
        loanAmountInput = 100.0;

    ImGui::Spacing();
    bool canBorrow = (loanAmountInput <= maxLoan);
    if (!canBorrow)
        ImGui::BeginDisabled();
    if (ImGui::Button("WEŹ KREDYT", ImVec2(160, 35)))
        bankSystem.takeLoan(loanAmountInput, bankBalance, calculateNetWorth());
    if (!canBorrow)
        ImGui::EndDisabled();

    ImGui::SameLine();
    bool canRepay = (bankSystem.getPlayerDebt() > 0.0 && bankBalance >= loanAmountInput);
    if (!canRepay)
        ImGui::BeginDisabled();
    if (ImGui::Button("SPŁAĆ KREDYT", ImVec2(160, 35)))
        bankSystem.repayLoan(loanAmountInput, bankBalance);
    if (!canRepay)
        ImGui::EndDisabled();

    ImGui::Columns(1);
    ImGui::EndChild();
}

void PlayingState::renderDarkwebTab()
{
    ImGui::BeginChild("DarkwebContent", ImVec2(0, 0), true);
    ImGui::TextColored(ImVec4(0.8f, 0.1f, 0.1f, 1.0f), "DARKWEB BLACK MARKET");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[POŁĄCZENIE TOR: SZYFROWANE]");
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Nielegalne oprogramowanie, sprzęty i usługi.");
    ImGui::Spacing();

    if (darkwebItems.empty())
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Brak towaru w asortymencie.");
    else
    {
        if (ImGui::BeginTable("TabelaDarkweb", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("Przedmiot", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Kategoria", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableSetupColumn("Opis", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Cena", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Akcja", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableHeadersRow();

            for (auto &item : darkwebItems)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", item.name.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%s", item.category.c_str());
                ImGui::TableNextColumn();
                ImGui::TextWrapped("%s", item.description.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%.2f PLN", item.price);

                ImGui::TableNextColumn();
                ImGui::PushID(item.id);
                if (item.isPurchased)
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[ ZAKUPIONO ]");
                else
                {
                    bool canAfford = (bankBalance >= item.price);
                    if (!canAfford)
                        ImGui::BeginDisabled();
                    if (ImGui::Button("KUP PRZEDMIOT", ImVec2(-1, 0)))
                        buyDarkwebItem(item.id);
                    if (!canAfford)
                        ImGui::EndDisabled();
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }
    ImGui::EndChild();
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
    std::string browserCheckboxLabel = "Przeglądarka Firewolf";
    if (unreadCount > 0)
        browserCheckboxLabel += " (" + std::to_string(unreadCount) + "!)";

    ImGui::Checkbox("Portfel & Bank", &showPortfolioPanel);
    ImGui::SameLine();
    ImGui::Checkbox(browserCheckboxLabel.c_str(), &showBrowserPanel);
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
        stats.isVictory = false;
        stats.endReason = "Ogłoszono upadłość na własne życzenie.";
        game->changeState(std::make_unique<BankruptcyState>(game, stats));
    }
    ImGui::End();

    if (showBrowserPanel)
        renderBrowserPanel();
    if (showPortfolioPanel)
        renderPortfolioPanel();

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
        ImGui::SetNextWindowSize(ImVec2(850, 600), ImGuiCond_FirstUseEver);
        ImGui::Begin("Analiza Wykresowa", &showChartPanel);

        ImGui::BeginChild("PanelFiltrow", ImVec2(200, 0), true);
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.8f, 1.0f), "Porównanie Akcji");
        ImGui::Separator();
        for (auto &company : companies)
            ImGui::Checkbox(company.ticker.c_str(), &company.showOnChart);
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Surowce");
        ImGui::Separator();
        for (auto &comm : commodities)
            ImGui::Checkbox(comm.symbol.c_str(), &comm.showOnChart);
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("ObszarWykresu", ImVec2(0, 0), false);
        ImGui::Checkbox("Śledź aktualny kurs (Okno 20h)", &autoScrollX);
        ImGui::Spacing();

        if (ImPlot::BeginPlot("Notowania Historyczne", ImVec2(-1, 320)))
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
                    targetX = timeHistory[closestIdx];
            }

            for (const auto &company : companies)
            {
                if (company.showOnChart && !timeHistory.empty())
                {
                    ImPlot::PlotLine(company.ticker.c_str(), timeHistory.data(), company.priceHistory.data(), (int)timeHistory.size());

                    if (isHovered && closestIdx != -1 && static_cast<size_t>(closestIdx) < company.priceHistory.size())
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

                    size_t fastPeriod = 15;
                    size_t slowPeriod = 40;

                    if (company.priceHistory.size() >= slowPeriod)
                    {
                        auto smaFast = calculateSMA(company.priceHistory, fastPeriod);
                        auto smaSlow = calculateSMA(company.priceHistory, slowPeriod);

                        std::string fastLabel = company.ticker + " SMA(" + std::to_string(fastPeriod) + ")";
                        std::string slowLabel = company.ticker + " SMA(" + std::to_string(slowPeriod) + ")";

                        ImPlot::PlotLine(fastLabel.c_str(), timeHistory.data(), smaFast.data(), (int)timeHistory.size());
                        ImPlot::PlotLine(slowLabel.c_str(), timeHistory.data(), smaSlow.data(), (int)timeHistory.size());

                        auto crosses = findCrossPoints(timeHistory, smaFast, smaSlow, slowPeriod);

                        for (const auto &cross : crosses)
                        {
                            if (cross.isGolden)
                            {
                                ImPlot::SetNextMarkerStyle(ImPlotMarker_Cross, 10.0f, ImVec4(1.0f, 0.84f, 0.0f, 1.0f), 1.0f, ImVec4(1.0f, 0.84f, 0.0f, 1.0f));
                                std::string label = "##GoldenCross_" + company.ticker + "_" + std::to_string(cross.x);
                                ImPlot::PlotScatter(label.c_str(), &cross.x, &cross.y, 1);
                            }
                            else
                            {
                                ImPlot::SetNextMarkerStyle(ImPlotMarker_Cross, 8.0f, ImVec4(1.0f, 0.2f, 0.2f, 1.0f), 1.0f, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                                std::string label = "##DeathCross_" + company.ticker + "_" + std::to_string(cross.x);
                                ImPlot::PlotScatter(label.c_str(), &cross.x, &cross.y, 1);
                            }
                        }
                    }
                }
            }

            for (const auto &comm : commodities)
            {
                if (comm.showOnChart && !timeHistory.empty())
                {
                    ImPlot::PlotLine(comm.symbol.c_str(), timeHistory.data(), comm.priceHistory.data(), (int)timeHistory.size());

                    if (isHovered && closestIdx != -1 && static_cast<size_t>(closestIdx) < comm.priceHistory.size())
                    {
                        double targetY = comm.priceHistory[closestIdx];
                        ImVec4 lineColor = ImPlot::GetLastItemColor();
                        ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, lineColor);
                        ImPlot::PushStyleColor(ImPlotCol_MarkerFill, lineColor);
                        ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Circle);
                        ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, 5.0f);
                        ImPlot::PlotScatter(("##dot_" + comm.name).c_str(), &targetX, &targetY, 1);
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
                    if (company.showOnChart && static_cast<size_t>(closestIdx) < company.priceHistory.size())
                        ImGui::Text("%s: %.2f PLN", company.ticker.c_str(), company.priceHistory[closestIdx]);
                }
                for (const auto &comm : commodities)
                {
                    if (comm.showOnChart && static_cast<size_t>(closestIdx) < comm.priceHistory.size())
                        ImGui::Text("%s: %.2f %s", comm.symbol.c_str(), comm.priceHistory[closestIdx], comm.unit.c_str());
                }
                ImGui::EndTooltip();
            }

            for (const auto &trade : tradeMarkers)
            {
                auto itComp = std::find_if(companies.begin(), companies.end(), [trade](const Company &c)
                                           { return c.id == trade.companyId; });

                if (itComp != companies.end() && itComp->showOnChart)
                {
                    ImVec2 mousePos = ImGui::GetMousePos();
                    ImVec2 markerPix = ImPlot::PlotToPixels(ImPlotPoint(trade.timeX, trade.priceY));
                    float dx = mousePos.x - markerPix.x;
                    float dy = mousePos.y - markerPix.y;
                    bool isHoveredMarker = ((dx * dx + dy * dy) <= 64.0f);

                    float currentMarkerSize = isHoveredMarker ? 8.0f : 4.0f;

                    if (trade.isBuy)
                    {
                        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, currentMarkerSize, ImVec4(0.2f, 1.0f, 0.2f, 0.9f), 1.0f, ImVec4(0.0f, 0.8f, 0.0f, 1.0f));
                    }
                    else
                    {
                        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, currentMarkerSize, ImVec4(1.0f, 0.3f, 0.3f, 0.9f), 1.0f, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));
                    }
                    std::string labelPrefix = trade.isBuy ? "##buy_" : "##sell_";
                    std::string markerLabel = labelPrefix + std::to_string(trade.companyId) + "_" + std::to_string(trade.timeX);
                    ImPlot::PlotScatter(markerLabel.c_str(), &trade.timeX, &trade.priceY, 1);

                    if (isHoveredMarker)
                    {
                        ImGui::BeginTooltip();
                        if (trade.isBuy)
                        {
                            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[ KUPNO: %s ]", itComp->ticker.c_str());
                            ImGui::Separator();
                            ImGui::Text("Ilość: %d szt.", trade.quantity);
                            ImGui::Text("Średnia cena zakupu: %.2f PLN", trade.buyPrice);
                            ImGui::Text("Data transakcji: %s", trade.dateStr.c_str());
                        }
                        else
                        {
                            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[ SPRZEDAŻ: %s ]", itComp->ticker.c_str());
                            ImGui::Separator();
                            ImGui::Text("Ilość: %d szt.", trade.quantity);
                            ImGui::Text("Średnia cena sprzedaży: %.2f PLN", trade.buyPrice);

                            if (trade.profitLoss >= 0.0)
                                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Wynik: +%.2f PLN (ZYSK)", trade.profitLoss);
                            else
                                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Wynik: %.2f PLN (STRATA)", trade.profitLoss);

                            ImGui::Text("Data transakcji: %s", trade.dateStr.c_str());
                        }
                        ImGui::EndTooltip();
                    }
                }
            }
            ImPlot::EndPlot();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "SZYBKI HANDEL ZAZNACZONYCH SPÓŁEK");
        ImGui::Spacing();

        bool anyCompanySelected = false;
        static std::map<int, int> chartTradeQuantities;
        static std::map<int, double> chartOrderTargetPrices;
        static std::map<int, int> chartOrderTypeIdx;

        for (auto &company : companies)
        {
            if (company.showOnChart)
            {
                anyCompanySelected = true;
                ImGui::PushID(company.id);

                if (chartTradeQuantities.find(company.id) == chartTradeQuantities.end())
                    chartTradeQuantities[company.id] = 1;

                if (chartOrderTargetPrices.find(company.id) == chartOrderTargetPrices.end() || chartOrderTargetPrices[company.id] <= 0.0)
                    chartOrderTargetPrices[company.id] = company.currentPrice;

                int &qty = chartTradeQuantities[company.id];
                double &targetPrice = chartOrderTargetPrices[company.id];
                int &orderType = chartOrderTypeIdx[company.id];

                int ownedShares = 0;
                auto itPos = portfolio.find(company.id);
                if (itPos != portfolio.end())
                    ownedShares = itPos->second.quantity;

                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[ %s ]", company.ticker.c_str());
                ImGui::SameLine();
                ImGui::Text("Kurs: %.2f PLN | Posiadasz: %d szt.", company.currentPrice, ownedShares);

                ImGui::PushItemWidth(90);
                ImGui::InputInt("Ilość", &qty);
                ImGui::PopItemWidth();
                if (qty < 1)
                    qty = 1;

                double stockCost = company.currentPrice * qty;
                double totalBuyCost = stockCost * (1.0 + suckersFeeRate);

                ImGui::SameLine();
                bool canBuy = (bankBalance >= totalBuyCost);
                if (!canBuy)
                    ImGui::BeginDisabled();
                if (ImGui::Button("KUP", ImVec2(75, 0)))
                    buyShares(company.id, qty);
                if (!canBuy)
                    ImGui::EndDisabled();

                if (ownedShares > 0)
                {
                    ImGui::SameLine();
                    bool canSell = (ownedShares >= qty);
                    if (!canSell)
                        ImGui::BeginDisabled();
                    if (ImGui::Button("SPRZEDAJ", ImVec2(80, 0)))
                        sellShares(company.id, std::min(qty, ownedShares));
                    if (!canSell)
                        ImGui::EndDisabled();

                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                    if (ImGui::Button("SPRZEDAJ WSZYSTKO", ImVec2(140, 0)))
                        sellShares(company.id, ownedShares);
                    ImGui::PopStyleColor(2);
                }

                ImGui::SameLine();
                ImGui::TextDisabled("|");

                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "Zlecenie:");

                ImGui::SameLine();
                ImGui::PushItemWidth(90);
                ImGui::InputDouble("##CenaLimit", &targetPrice, 0.0, 0.0, "%.2f");
                ImGui::PopItemWidth();
                if (targetPrice < 0.01)
                    targetPrice = 0.01;

                ImGui::SameLine();
                ImGui::PushItemWidth(110);
                const char *orderTypes[] = {"Limit Buy", "Limit Sell", "Stop Loss"};
                ImGui::Combo("##TypZlecenia", &orderType, orderTypes, IM_ARRAYSIZE(orderTypes));
                ImGui::PopItemWidth();

                ImGui::SameLine();
                if (ImGui::Button("+ ZLECENIE", ImVec2(90, 0)))
                {
                    OrderType type = OrderType::LimitBuy;
                    if (orderType == 1)
                        type = OrderType::LimitSell;
                    else if (orderType == 2)
                        type = OrderType::StopLoss;

                    addPendingOrder(company.id, type, qty, targetPrice);
                }

                ImGui::Separator();
                ImGui::PopID();
            }
        }

        if (!anyCompanySelected)
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Zaznacz spółkę na liście po lewej stronie, aby pojawiły się opcje szybkiego handlu.");

        ImGui::EndChild();
        ImGui::End();
    }

    if (showDetailsPanel)
    {
        ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, pivotCenter);
        ImGui::SetNextWindowSize(ImVec2(520, 500), ImGuiCond_FirstUseEver);
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
                    ownedShares = itPos->second.quantity;

                ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "SZYBKI HANDEL NATYCHMIASTOWY");
                ImGui::Text("Posiadane akcje: %d szt.", ownedShares);

                static int tradeQty = 1;
                ImGui::PushItemWidth(120);
                ImGui::InputInt("Ilość akcji##Direct", &tradeQty);
                ImGui::PopItemWidth();
                if (tradeQty < 1)
                    tradeQty = 1;

                bool canAfford = (bankBalance >= selected->currentPrice * tradeQty * (1.0 + suckersFeeRate));
                if (!canAfford)
                    ImGui::BeginDisabled();
                if (ImGui::Button("KUP TERAZ", ImVec2(100, 30)))
                    buyShares(selected->id, tradeQty);
                if (!canAfford)
                    ImGui::EndDisabled();

                if (ownedShares > 0)
                {
                    ImGui::SameLine();
                    if (ImGui::Button("SPRZEDAJ TERAZ", ImVec2(120, 30)))
                        sellShares(selected->id, std::min(tradeQty, ownedShares));
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "ZLECENIA OCZEKUJĄCE (LIMIT / STOP-LOSS)");
                ImGui::Spacing();

                static int orderQty = 1;
                static double orderTargetPrice = 100.0;
                static int orderTypeIdx = 0;

                ImGui::PushItemWidth(120);
                ImGui::InputInt("Ilość##Order", &orderQty);
                if (orderQty < 1)
                    orderQty = 1;

                ImGui::InputDouble("Cena Limit/SL##Order", &orderTargetPrice, 1.0, 10.0, "%.2f");
                if (orderTargetPrice < 0.01)
                    orderTargetPrice = 0.01;

                const char *orderTypes[] = {"LIMIT BUY (Kup gdy cena <= Target)", "LIMIT SELL (Sprzedaj gdy cena >= Target)", "STOP LOSS (Sprzedaj gdy cena <= Target)"};
                ImGui::Combo("Typ Zlecenia", &orderTypeIdx, orderTypes, IM_ARRAYSIZE(orderTypes));
                ImGui::PopItemWidth();

                ImGui::Spacing();
                if (ImGui::Button("ZŁÓŻ ZLECENIE OCZEKUJĄCE", ImVec2(220, 32)))
                {
                    OrderType type = OrderType::LimitBuy;
                    if (orderTypeIdx == 1)
                        type = OrderType::LimitSell;
                    else if (orderTypeIdx == 2)
                        type = OrderType::StopLoss;

                    addPendingOrder(selected->id, type, orderQty, orderTargetPrice);
                }
            }
        }
        else
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Wybierz spółkę z listy 'Notowania Spółek', aby wyświetlić raport.");

        ImGui::End();
    }

    renderVictoryModal();
    renderPieChartsWindow();
}