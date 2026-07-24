#pragma once
#include "Company.hpp"
#include "Commodity.hpp"
#include "Sector.hpp"
#include "Mail.hpp"
#include <string>
#include <vector>
#include <random>
#include <memory>
#include <nlohmann/json.hpp>

class Game;

enum class EventCategory
{
    Global,
    Company,
    Private
};

NLOHMANN_JSON_SERIALIZE_ENUM(EventCategory, {
    {EventCategory::Global, "Global"},
    {EventCategory::Company, "Company"},
    {EventCategory::Private, "Private"}
})

struct EventTemplate
{
    EventCategory category = EventCategory::Global;
    std::string sender;
    std::string subject;
    std::string body;

    std::string targetType;
    Sector targetSector = Sector::Tech;
    std::string targetTicker;
    std::string targetSymbol;
    double trendModifier = 0.0;
    double durationHours = 0.0;

    double amount = 0.0;
    bool isBill = false; 
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    EventTemplate,
    category, sender, subject, body, targetType, targetSector,
    targetTicker, targetSymbol, trendModifier, durationHours, amount, isBill
)

struct ActiveMarketEffect
{
    std::string name;
    EventCategory category;
    int targetId = -1;
    Sector targetSector = Sector::Tech;
    bool isSector = false;
    bool isCommodity = false;
    double trendModifier = 0.0;
    double hoursRemaining = 0.0;
};

class EventSystem
{
private:
    std::vector<EventTemplate> templates;
    std::vector<ActiveMarketEffect> activeEffects;
    std::mt19937 rng;

    void replaceAll(std::string& str, const std::string& from, const std::string& to);

public:
    EventSystem();

    bool loadEventsFromJson(const std::string& filepath);

    void triggerGlobalEvent(std::vector<Commodity>& commodities, std::vector<Company>& companies, std::vector<MailMessage>& inbox, int& nextMailId, const std::string& timestamp);
    void triggerCompanyEvent(std::vector<Company>& companies, std::vector<MailMessage>& inbox, int& nextMailId, const std::string& timestamp);
    void triggerPrivateEvent(double& bankBalance, std::vector<MailMessage>& inbox, int& nextMailId, const std::string& timestamp);

    void generateWeeklyLivingBill(double netWorth, std::vector<MailMessage>& inbox, int& nextMailId, const std::string& timestamp);
    bool processUnpaidBills(double& bankBalance, std::vector<MailMessage>& inbox, int& nextMailId, const std::string& timestamp, Game* game);

    void update(double dtHours);
    void checkAndTriggerRandomEvent(std::vector<Company>& companies, std::vector<Commodity>& commodities, double& bankBalance, std::vector<MailMessage>& inbox, int& nextMailId, const std::string& timestamp);

    double getCompanyTrendModifier(int companyId, Sector sector) const;
    double getCommodityTrendModifier(int commodityId) const;
};