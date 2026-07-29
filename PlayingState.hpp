#pragma once
#include "GameState.hpp"
#include "Company.hpp"
#include "Commodity.hpp"
#include "Mail.hpp"
#include "DarkwebItem.hpp"
#include "EventSystem.hpp"
#include "BankSystem.hpp"
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

struct RunStats
{
    int daysSurvived = 0;
    double maxNetWorth = 10000.0;
    double maxSingleProfit = 0.0;
    double totalFeesPaid = 0.0;
    double totalInterestPaid = 0.0;
    int totalTrades = 0;
    bool isVictory = false;
    std::string endReason = "";
};

struct PortfolioPosition
{
    int companyId = 0;
    int quantity = 0;
    double avgBuyPrice = 0.0;
};

struct TradeMarker
{
    int companyId = 0;
    double timeX = 0.0;
    double priceY = 0.0;
    int quantity = 0;
    double buyPrice = 0.0;
    std::string dateStr;
    bool isBuy = true;
    double profitLoss = 0.0;
};

enum class OrderType
{
    LimitBuy,
    LimitSell,
    StopLoss
};

struct PendingOrder
{
    int id = 0;
    int companyId = 0;
    OrderType type = OrderType::LimitBuy;
    int quantity = 1;
    double targetPrice = 0.0;
};

class PlayingState : public GameState
{
private:
    std::vector<Company> companies;
    std::vector<Commodity> commodities;
    std::vector<DarkwebItem> darkwebItems;
    std::optional<int> selectedCompanyId;

    double bankBalance = 10000.00;
    double suckersFeeRate = 0.02;
    std::unordered_map<int, PortfolioPosition> portfolio;

    std::vector<PendingOrder> pendingOrders;
    int nextOrderId = 1;

    std::vector<MailMessage> inbox;
    std::optional<int> selectedMailId;
    int nextMailId = 1;
    int daysPassedCounter = 0;
    int lastRecordedDay = 0;

    EventSystem eventSystem;
    GameTime gameTime;

    float timeSpeedMultiplier = 1.0f;
    float savedSpeedMultiplier = 1.0f;

    std::vector<double> timeHistory;
    double totalSimulatedHours = 0;

    bool showCompaniesPanel = false;
    bool showCommoditiesPanel = false;
    bool showDetailsPanel = false;
    bool showChartPanel = false;
    bool showPortfolioPanel = true;
    bool showBrowserPanel = true;
    bool showPieChartsWindow = false;

    int activeBrowserTab = 0;
    bool autoScrollX = true;

    static constexpr size_t MAX_HISTORY_SIZE = 5500;
    std::vector<TradeMarker> tradeMarkers;
    BankSystem bankSystem;
    double loanAmountInput = 3000.0;

    RunStats stats;
    bool isFreeplay = false;
    bool showVictoryModal = false;

    double getQuarterlyQuota(int year, int quarter) const
    {
        if (year > 1)
            return 1000000.0;
        switch (quarter)
        {
        case 1:
            return 15000.0;
        case 2:
            return 50000.0;
        case 3:
            return 150000.0;
        case 4:
            return 1000000.0;
        default:
            return 1000000.0;
        }
    }

    void checkQuarterlyQuota();
    void renderVictoryModal();

    void renderBrowserPanel();
    void renderMailTab();
    void renderBankTab();
    void renderDarkwebTab();
    void renderPieChartsWindow();

    void loadDarkwebItems();
    bool buyDarkwebItem(int itemId);

    void processPendingOrders();
    void addPendingOrder(int companyId, OrderType type, int quantity, double targetPrice);
    void cancelPendingOrder(int orderId);

    void generateStartingCompanies();
    void generateStartingCommodities();
    void renderClock();
    void renderPortfolioPanel();

    double calculateNetWorth() const;
    bool payBill(int billId);

    bool buyShares(int companyId, int quantity);
    bool sellShares(int companyId, int quantity);

public:
    PlayingState(Game *game);

    void handleEvent(const sf::Event &event) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow &window) override;
    void renderImGui() override;
};