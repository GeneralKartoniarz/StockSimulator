#pragma once
#include "GameState.hpp"
#include "Company.hpp"
#include "Commodity.hpp"
#include "Mail.hpp"
#include "EventSystem.hpp"
#include <vector>
#include <unordered_map>
#include <optional>
#include <string>

struct GameTime
{
    int year = 1;
    int quarter = 1;
    int monthInQuarter = 1;
    int day = 1;
    int hour = 9;
    int minute = 0;
    float accumulator = 0.0f;
    bool update(float dt);
};

struct PortfolioPosition
{
    int companyId = 0;
    int quantity = 0;
    double avgBuyPrice = 0.0;
};

class PlayingState : public GameState
{
private:
    std::vector<Company> companies;
    std::vector<Commodity> commodities;
    std::optional<int> selectedCompanyId;
    
    double bankBalance = 10000.00;
    double suckersFeeRate = 0.02;
    std::unordered_map<int, PortfolioPosition> portfolio;
    int tradeQuantity = 1;

    std::vector<MailMessage> inbox;
    std::optional<int> selectedMailId;
    int nextMailId = 1;
    int daysPassedCounter = 0;
    int lastRecordedDay = 1;

    EventSystem eventSystem;

    GameTime gameTime;

    std::vector<double> timeHistory;
    double totalSimulatedHours = 0;

    bool showCompaniesPanel = false;
    bool showCommoditiesPanel = false;
    bool showDetailsPanel = false;
    bool showChartPanel = false;
    bool showPortfolioPanel = true;
    bool showMailPanel = true;

    bool autoScrollX = true;

    static constexpr size_t MAX_HISTORY_SIZE = 1300;

    void generateStartingCompanies();
    void generateStartingCommodities();
    void renderClock();
    void renderPortfolioPanel();
    void renderMailPanel();

    double calculateNetWorth() const;
    bool payBill(int billId);

    bool buyShares(int companyId, int quantity);
    bool sellShares(int companyId, int quantity);

public:
    PlayingState(Game* game);

    void handleEvent(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void renderImGui() override;
};