#pragma once
#include <string>

enum class MailType
{
    Bill,   // Rachunek do zapłaty
    Event,  // Wydarzenie osobiste (np. stłuczka)
    News    // Wiadomości giełdowe / rynkowe
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
};