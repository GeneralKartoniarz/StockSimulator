#include "BankSystem.hpp"
#include <algorithm>
#include <cmath>

BankSystem::BankSystem() : rng(std::random_device{}()) {}

double BankSystem::calculateMaxLoan(double netWorth) const
{
    double maxCreditLimit = netWorth * creditScoreMultiplier;
    return std::max(0.0, maxCreditLimit - playerDebt);
}

double BankSystem::calculateWeeklyInterest() const
{
    double annualRate = getLoanInterestRate();
    return playerDebt * (annualRate / 52.0);
}

bool BankSystem::takeLoan(double amount, double& bankBalance, double netWorth)
{
    if (amount <= 0.0) return false;
    double maxAllowed = calculateMaxLoan(netWorth);

    if (amount > maxAllowed) return false;

    playerDebt += amount;
    bankBalance += amount;
    return true;
}

bool BankSystem::repayLoan(double amount, double& bankBalance)
{
    if (amount <= 0.0 || playerDebt <= 0.0) return false;
    if (bankBalance < amount) return false;

    double actualRepayment = std::min(amount, playerDebt);
    bankBalance -= actualRepayment;
    playerDebt -= actualRepayment;

    successfulRepaymentsCount++;
    if (successfulRepaymentsCount >= 4 && creditScoreMultiplier < 1.5)
    {
        successfulRepaymentsCount = 0;
        creditScoreMultiplier += 0.1;
    }

    return true;
}

void BankSystem::processWeeklyInterest(double& bankBalance, std::vector<MailMessage>& inbox, int& nextMailId, const std::string& timestamp)
{
    if (playerDebt <= 0.0) return;

    double interestCost = calculateWeeklyInterest();

    if (bankBalance >= interestCost)
    {
        bankBalance -= interestCost;

        MailMessage mail;
        mail.id = nextMailId++;
        mail.type = MailType::News;
        mail.sender = "PKO Bank Centralny";
        mail.subject = "Pobrano tygodniowe odsetki od kredytu";
        mail.timestamp = timestamp;
        mail.body = "Z Twojego konta pobrano kwotę " + std::to_string((int)interestCost) + 
                    " PLN z tytułu odsetek od pobranego długu (" + std::to_string((int)playerDebt) + " PLN).";
        inbox.push_back(mail);
    }
    else
    {
        MailMessage mail;
        mail.id = nextMailId++;
        mail.type = MailType::Bill;
        mail.sender = "PKO Bank Centralny - Dział Ratalny";
        mail.subject = "RACHUNEK: Zaległe odsetki od kredytu";
        mail.timestamp = timestamp;
        mail.amount = interestCost;
        mail.body = "Brak wystarczających środków na automatyczne pokrycie odsetek kredytowych! "
                    "Prosimy o uregulowanie kwoty " + std::to_string((int)interestCost) + " PLN.";
        inbox.push_back(mail);
    }
}

void BankSystem::updateMacroeconomy(std::vector<MailMessage>& inbox, int& nextMailId, const std::string& timestamp)
{
    std::normal_distribution<double> inflShift(0.0, 0.005);
    double shift = inflShift(rng);

    inflationRate = std::clamp(inflationRate + shift, 0.01, 0.18);
    
    double targetBaseRate = inflationRate + 0.015;
    double oldRate = baseInterestRate;
    baseInterestRate = targetBaseRate;

    if (std::abs(baseInterestRate - oldRate) >= 0.005)
    {
        MailMessage mail;
        mail.id = nextMailId++;
        mail.type = MailType::News;
        mail.sender = "Rada Polityki Pieniężnej";
        mail.subject = "Decyzja RPP: Zmiana stóp procentowych!";
        mail.timestamp = timestamp;
        
        char buffer[256];
        snprintf(buffer, sizeof(buffer), 
                 "W odpowiedzi na inflację (%.1f%%) Bank Centralny zmienił stopę referencyjną z %.1f%% na %.1f%%. "
                 "Oprocentowanie Twoich kredytów wynosi teraz %.1f%%.",
                 inflationRate * 100.0, oldRate * 100.0, baseInterestRate * 100.0, getLoanInterestRate() * 100.0);
        
        mail.body = buffer;
        inbox.push_back(mail);
    }
}