#pragma once
#include "Sector.hpp"
#include <string>

enum class MailType
{
    Bill,      // Rachunek do zapłaty
    Event,     // Wydarzenie osobiste (np. stłuczka)
    News,      // Wiadomości rynkowe
    TipOffer   // Oferta zakupu przecieku / informacji insidera
};

struct MailMessage
{
    int id = 0;
    MailType type = MailType::Bill;
    std::string sender;
    std::string subject;
    std::string body;
    std::string timestamp;
    double amount = 0.0;
    bool isRead = false;
    bool isResolved = false;
    int unpaidWeeks = 0;
    int parentBillId = 0;

    int tipDelayDays = 0;
    double tipTrendModifier = 0.0;
    double tipDurationHours = 0.0;
    
    std::string tipSubject;
    std::string tipBody;
    std::string tipSuccessBody;
    std::string tipFailBody;

    int tipTargetId = -1;
    Sector tipTargetSector = Sector::Tech;
    bool tipIsSector = false;
    bool tipIsCommodity = false;
    bool tipIsCompany = false;
};