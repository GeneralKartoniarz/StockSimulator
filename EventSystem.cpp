#include "EventSystem.hpp"
#include "Game.hpp"
#include "MainMenuState.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include "BankruptcyState.hpp"
EventSystem::EventSystem() : rng(std::random_device{}()) {}

void EventSystem::replaceAll(std::string &str, const std::string &from, const std::string &to)
{
    if (from.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

bool EventSystem::loadEventsFromJson(const std::string &filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "[BLAD EventSystem] Nie udalo sie otworzyc " << filepath << "!\n";
        return false;
    }

    try
    {
        nlohmann::json j;
        file >> j;
        templates = j.get<std::vector<EventTemplate>>();
        std::cout << "[INFO EventSystem] Wczytano " << templates.size() << " szablonow z JSON.\n";
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[BLAD EventSystem JSON] " << e.what() << "\n";
        return false;
    }
}

void EventSystem::update(double dtHours)
{
    for (auto &effect : activeEffects)
    {
        effect.hoursRemaining -= dtHours;
    }

    std::erase_if(activeEffects, [](const ActiveMarketEffect &e)
                  { return e.hoursRemaining <= 0.0; });
}

double EventSystem::getCompanyTrendModifier(int companyId, Sector sector) const
{
    double totalMod = 0.0;
    for (const auto &effect : activeEffects)
    {
        if (effect.category == EventCategory::Company && effect.targetId == companyId)
        {
            totalMod += effect.trendModifier;
        }
        else if (effect.category == EventCategory::Global && effect.isSector && effect.targetSector == sector)
        {
            totalMod += effect.trendModifier;
        }
    }
    return totalMod;
}

double EventSystem::getCommodityTrendModifier(int commodityId) const
{
    double totalMod = 0.0;
    for (const auto &effect : activeEffects)
    {
        if (effect.category == EventCategory::Global && effect.isCommodity && effect.targetId == commodityId)
        {
            totalMod += effect.trendModifier;
        }
    }
    return totalMod;
}

void EventSystem::generateWeeklyLivingBill(double netWorth, std::vector<MailMessage> &inbox, int &nextMailId, const std::string &timestamp)
{
    double billAmount = 100.0 + (netWorth * 0.015);

    MailMessage mail;
    mail.id = nextMailId++;
    mail.type = MailType::Bill;
    mail.sender = "Tauron & Wodociągi SA";
    mail.subject = "Faktura za koszty utrzymania";
    mail.timestamp = timestamp;
    mail.amount = billAmount;
    mail.body = "Szanowny Kliencie,\n\n"
                "Przesyłamy zestawienie opłat eksploatacyjnych za miniony tydzień. "
                "Wysokość rachunku została dostosowana do Twojego aktualnego standardu życia.\n\n"
                "Prosimy o uregulowanie płatności w terminie.";

    inbox.push_back(mail);
}

bool EventSystem::processUnpaidBills(double &bankBalance, std::vector<MailMessage> &inbox, int &nextMailId, const std::string &timestamp, Game *game)
{
    std::vector<MailMessage> newMails;
    size_t currentInboxSize = inbox.size();

    for (size_t i = 0; i < currentInboxSize; ++i)
    {
        auto &mail = inbox[i];
        if (mail.type == MailType::Bill && !mail.isResolved)
        {
            mail.unpaidWeeks++;

            if (mail.unpaidWeeks == 1)
            {
                MailMessage reminder;
                reminder.id = nextMailId++;
                reminder.type = MailType::News;
                reminder.sender = "E-Sąd & Kancelaria Prawna";
                reminder.subject = "PONOWNE ZAWIADOMIENIE: Rachunek #" + std::to_string(mail.id);
                reminder.timestamp = timestamp;
                reminder.parentBillId = mail.id;
                reminder.body = "Przypominamy, że opłata (" + mail.subject + ") na kwotę " +
                                std::to_string((int)mail.amount) + " PLN nie została uregulowana. Prosimy o natychmiastową wpłatę.";
                newMails.push_back(reminder);
            }
            else if (mail.unpaidWeeks == 2)
            {
                MailMessage warning;
                warning.id = nextMailId++;
                warning.type = MailType::News;
                warning.sender = "Dział Egzekucji i Windykacji";
                warning.subject = "OSTATECZNE WEZWANIE: Rachunek #" + std::to_string(mail.id);
                warning.timestamp = timestamp;
                warning.parentBillId = mail.id;
                warning.body = "OSTATECZNE WEZWANIE DO ZAPŁATY! Rachunek #" + std::to_string(mail.id) +
                               " nie został opłacony. W następnym tygodniu sprawa trafi do Komornika z 10% opłatą karną.";
                newMails.push_back(warning);
            }
            else if (mail.unpaidWeeks >= 3)
            {
                double totalPenaltyCost = mail.amount * 1.10;

                if (bankBalance >= totalPenaltyCost)
                {
                    bankBalance -= totalPenaltyCost;
                    mail.isResolved = true;

                    int targetId = mail.id;
                    std::erase_if(inbox, [targetId](const MailMessage &m)
                                  { return m.parentBillId == targetId; });

                    MailMessage execution;
                    execution.id = nextMailId++;
                    execution.type = MailType::News;
                    execution.sender = "Komornik Sądowy";
                    execution.subject = "PRZYMUSOWA EGZEKUCJA: Rachunek #" + std::to_string(mail.id);
                    execution.timestamp = timestamp;
                    execution.body = "Z Twojego konta pobrano kwotę " +
                                     std::to_string((int)totalPenaltyCost) + " PLN (w tym 10% kary) za nieopłacony rachunek #" + std::to_string(mail.id) + ".";
                    newMails.push_back(execution);
                }
                else
                {
                    game->changeState(std::make_unique<BankruptcyState>(game));
                    return false;
                }
            }
        }
    }

    inbox.insert(inbox.end(), newMails.begin(), newMails.end());
    return true;
}

void EventSystem::triggerGlobalEvent(std::vector<Commodity> &commodities, std::vector<Company> &companies, std::vector<MailMessage> &inbox, int &nextMailId, const std::string &timestamp)
{
    std::vector<EventTemplate> matches;
    for (const auto &t : templates)
    {
        if (t.category == EventCategory::Global)
            matches.push_back(t);
    }
    if (matches.empty())
        return;

    std::uniform_int_distribution<size_t> dist(0, matches.size() - 1);
    const auto &tmpl = matches[dist(rng)];

    ActiveMarketEffect effect;
    effect.name = tmpl.subject;
    effect.category = EventCategory::Global;
    effect.trendModifier = tmpl.trendModifier;
    effect.hoursRemaining = tmpl.durationHours;

    if (tmpl.targetType == "Commodity")
    {
        auto it = std::find_if(commodities.begin(), commodities.end(), [&](const Commodity &c)
                               { return c.symbol == tmpl.targetSymbol; });
        if (it != commodities.end())
        {
            effect.isCommodity = true;
            effect.targetId = it->id;
            activeEffects.push_back(effect);
        }
    }
    else if (tmpl.targetType == "Sector")
    {
        effect.isSector = true;
        effect.targetSector = tmpl.targetSector;
        activeEffects.push_back(effect);
    }

    MailMessage mail;
    mail.id = nextMailId++;
    mail.type = MailType::News;
    mail.sender = tmpl.sender;
    mail.subject = tmpl.subject;
    mail.body = tmpl.body;
    mail.timestamp = timestamp;
    inbox.push_back(mail);
}

void EventSystem::triggerCompanyEvent(std::vector<Company> &companies, std::vector<MailMessage> &inbox, int &nextMailId, const std::string &timestamp)
{
    if (companies.empty())
        return;

    std::vector<EventTemplate> matches;
    for (const auto &t : templates)
    {
        if (t.category == EventCategory::Company)
            matches.push_back(t);
    }
    if (matches.empty())
        return;

    std::uniform_int_distribution<size_t> dist(0, matches.size() - 1);
    const auto &tmpl = matches[dist(rng)];

    const Company *targetComp = nullptr;

    if (tmpl.targetType == "Company")
    {
        auto it = std::find_if(companies.begin(), companies.end(), [&](const Company &c)
                               { return c.ticker == tmpl.targetTicker; });
        if (it != companies.end())
            targetComp = &(*it);
    }
    else if (tmpl.targetType == "RandomCompany")
    {
        std::uniform_int_distribution<size_t> cDist(0, companies.size() - 1);
        targetComp = &companies[cDist(rng)];
    }

    if (!targetComp)
        return;

    ActiveMarketEffect effect;
    effect.name = tmpl.subject;
    effect.category = EventCategory::Company;
    effect.targetId = targetComp->id;
    effect.trendModifier = tmpl.trendModifier;
    effect.hoursRemaining = tmpl.durationHours;
    activeEffects.push_back(effect);

    std::string subject = tmpl.subject;
    std::string body = tmpl.body;

    replaceAll(subject, "{COMPANY_NAME}", targetComp->name);
    replaceAll(subject, "{COMPANY_TICKER}", targetComp->ticker);
    replaceAll(body, "{COMPANY_NAME}", targetComp->name);
    replaceAll(body, "{COMPANY_TICKER}", targetComp->ticker);

    MailMessage mail;
    mail.id = nextMailId++;
    mail.type = MailType::News;
    mail.sender = tmpl.sender;
    mail.subject = subject;
    mail.body = body;
    mail.timestamp = timestamp;
    inbox.push_back(mail);
}

void EventSystem::triggerPrivateEvent(double &bankBalance, std::vector<MailMessage> &inbox, int &nextMailId, const std::string &timestamp)
{
    std::vector<EventTemplate> matches;
    for (const auto &t : templates)
    {
        if (t.category == EventCategory::Private)
            matches.push_back(t);
    }
    if (matches.empty())
        return;

    std::uniform_int_distribution<size_t> dist(0, matches.size() - 1);
    const auto &tmpl = matches[dist(rng)];

    std::string body = tmpl.body;
    replaceAll(body, "{AMOUNT}", std::to_string((int)tmpl.amount));

    if (tmpl.isBill)
    {
        MailMessage mail;
        mail.id = nextMailId++;
        mail.type = MailType::Bill;
        mail.sender = tmpl.sender;
        mail.subject = tmpl.subject;
        mail.body = body;
        mail.timestamp = timestamp;
        mail.amount = tmpl.amount;
        inbox.push_back(mail);
    }
    else
    {
        bankBalance += tmpl.amount;

        MailMessage mail;
        mail.id = nextMailId++;
        mail.type = MailType::Event;
        mail.sender = tmpl.sender;
        mail.subject = tmpl.subject;
        mail.body = body;
        mail.timestamp = timestamp;
        inbox.push_back(mail);
    }
}

void EventSystem::checkAndTriggerRandomEvent(std::vector<Company> &companies, std::vector<Commodity> &commodities, double &bankBalance, std::vector<MailMessage> &inbox, int &nextMailId, const std::string &timestamp)
{
    std::uniform_real_distribution<double> chance(0.0, 1.0);
    if (chance(rng) < 0.25)
    {
        std::uniform_int_distribution<int> catDist(0, 2);
        int cat = catDist(rng);

        if (cat == 0)
            triggerGlobalEvent(commodities, companies, inbox, nextMailId, timestamp);
        else if (cat == 1)
            triggerCompanyEvent(companies, inbox, nextMailId, timestamp);
        else
            triggerPrivateEvent(bankBalance, inbox, nextMailId, timestamp);
    }
}
void EventSystem::triggerTipOfferEvent(std::vector<Company> &companies, std::vector<Commodity> &commodities, std::vector<MailMessage> &inbox, int &nextMailId, const std::string &timestamp)
{
    std::vector<EventTemplate> matches;
    for (const auto &t : templates)
    {
        if (t.category == EventCategory::TipOffer)
            matches.push_back(t);
    }
    if (matches.empty())
        return;

    std::uniform_int_distribution<size_t> dist(0, matches.size() - 1);
    const auto &tmpl = matches[dist(rng)];

    std::uniform_int_distribution<int> delayDist(tmpl.minDelayDays, tmpl.maxDelayDays);
    int delayDays = delayDist(rng);

    MailMessage mail;
    mail.id = nextMailId++;
    mail.type = MailType::TipOffer;
    mail.sender = tmpl.sender;
    mail.subject = tmpl.subject;
    mail.amount = tmpl.amount;
    mail.timestamp = timestamp;

    std::string body = tmpl.body;
    replaceAll(body, "{AMOUNT}", std::to_string((int)tmpl.amount));
    mail.body = body;

    mail.tipTrendModifier = tmpl.trendModifier;
    mail.tipDurationHours = tmpl.durationHours;
    mail.tipDelayDays = delayDays;

    if (tmpl.targetType == "RandomCompany" && !companies.empty())
    {
        std::uniform_int_distribution<size_t> cDist(0, companies.size() - 1);
        const auto &targetComp = companies[cDist(rng)];

        std::string succBody = tmpl.successBody;
        std::string failBody = tmpl.failBody;
        replaceAll(succBody, "{COMPANY_NAME}", targetComp.name);
        replaceAll(failBody, "{COMPANY_NAME}", targetComp.name);

        mail.tipIsCompany = true;
        mail.tipTargetId = targetComp.id;
        mail.tipSubject = targetComp.name;
        mail.tipSuccessBody = succBody;
        mail.tipFailBody = failBody;
    }

    inbox.push_back(mail);
}
bool EventSystem::buyTip(int mailId, double &bankBalance, std::vector<MailMessage> &inbox, int &nextMailId, const std::string &timestamp, const std::vector<Company> &companies)
{
    auto it = std::find_if(inbox.begin(), inbox.end(), [mailId](const MailMessage &m)
                           { return m.id == mailId; });

    if (it == inbox.end() || it->isResolved)
        return false;

    if (bankBalance >= it->amount)
    {
        bankBalance -= it->amount;
        it->isResolved = true;

        std::uniform_real_distribution<double> chance(0.0, 1.0);
        bool isTrue = (chance(rng) < 0.50);

        PendingEvent pending;
        pending.daysRemaining = it->tipDelayDays;
        pending.isTrue = isTrue;
        pending.sender = it->sender;
        pending.subject = "WYNIK PRZECIEKU: " + it->tipSubject;
        pending.successBody = it->tipSuccessBody;
        pending.failBody = it->tipFailBody;

        pending.effect.name = "Efekt przecieku: " + it->tipSubject;
        pending.effect.category = EventCategory::Company;
        pending.effect.trendModifier = it->tipTrendModifier;
        pending.effect.hoursRemaining = it->tipDurationHours;

        if (it->tipIsCompany)
        {
            pending.effect.targetId = it->tipTargetId;
        }
        else if (it->tipIsSector)
        {
            pending.effect.isSector = true;
            pending.effect.targetSector = it->tipTargetSector;
        }

        pendingEvents.push_back(pending);

        MailMessage ackMail;
        ackMail.id = nextMailId++;
        ackMail.type = MailType::News;
        ackMail.sender = it->sender;
        ackMail.subject = "TRANSAKCJA PRZYJĘTA: " + it->tipSubject;
        ackMail.timestamp = timestamp;
        ackMail.body = "Otrzymano wpłatę. Informacja została przekazana do weryfikacji.\n"
                       "Oczekuj raportu w skrzynce za około " +
                       std::to_string(it->tipDelayDays) + " dni.";

        inbox.push_back(ackMail);
        return true;
    }
    return false;
}

void EventSystem::processPendingEvents(std::vector<MailMessage> &inbox, int &nextMailId, const std::string &timestamp)
{
    for (auto it = pendingEvents.begin(); it != pendingEvents.end();)
    {
        it->daysRemaining--;

        if (it->daysRemaining <= 0)
        {
            MailMessage resultMail;
            resultMail.id = nextMailId++;
            resultMail.type = MailType::News;
            resultMail.sender = it->sender;
            resultMail.subject = it->subject;
            resultMail.timestamp = timestamp;

            if (it->isTrue)
            {
                activeEffects.push_back(it->effect);
                resultMail.body = it->successBody;
            }
            else
            {
                resultMail.body = it->failBody;
            }

            inbox.push_back(resultMail);
            it = pendingEvents.erase(it);
        }
        else
        {
            ++it;
        }
    }
}