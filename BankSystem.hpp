#pragma once
#include "Mail.hpp"
#include <string>
#include <vector>
#include <random>

class BankSystem
{
private:
    double inflationRate = 0.045;    
    double baseInterestRate = 0.060;  
    double bankMargin = 0.035;        
    
    double playerDebt = 0.0;          
    double creditScoreMultiplier = 0.8;
    int successfulRepaymentsCount = 0;

    std::mt19937 rng;

public:
    BankSystem();

    double getInflationRate() const { return inflationRate; }
    double getBaseInterestRate() const { return baseInterestRate; }
    double getLoanInterestRate() const { return baseInterestRate + bankMargin; }
    double getPlayerDebt() const { return playerDebt; }
    double getCreditScoreMultiplier() const { return creditScoreMultiplier; }

    double calculateMaxLoan(double netWorth) const;
    double calculateWeeklyInterest() const;

    bool takeLoan(double amount, double& bankBalance, double netWorth);
    bool repayLoan(double amount, double& bankBalance);

    void processWeeklyInterest(double& bankBalance, std::vector<MailMessage>& inbox, int& nextMailId, const std::string& timestamp);
    void updateMacroeconomy(std::vector<MailMessage>& inbox, int& nextMailId, const std::string& timestamp);
};