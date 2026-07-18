#pragma once
#include <string>
#include <vector>
struct Company {
    int id;
    std::string ticker;
    std::string name;        
    std::string description; 
    double currentPrice;   
    std::vector<double> priceHistory;
    bool showOnChart = false;
    
};