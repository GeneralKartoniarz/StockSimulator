#pragma once
#include "GameState.hpp"
#include "Company.hpp"
#include "Commodity.hpp"
#include <vector>
#include <optional>

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

class PlayingState : public GameState
{
private:
    std::vector<Company> companies;
    std::vector<Commodity> commodities;
    std::optional<int> selectedCompanyId;
    
    GameTime gameTime;

    std::vector<double> timeHistory;
    double totalSimulatedHours = 0;

    bool showCompaniesPanel = false;
    bool showCommoditiesPanel = false;
    bool showDetailsPanel = false;
    bool showChartPanel = false;

    bool autoScrollX = true;

    static constexpr size_t MAX_HISTORY_SIZE = 300;

    void generateStartingCompanies();
    void generateStartingCommodities();
    void renderClock();

public:
    PlayingState(Game* game);

    void handleEvent(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void renderImGui() override;
};