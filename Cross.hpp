#pragma once
#include <vector>
#include <cstddef>

struct CrossPoint {
    double x;        // Czas (totalSimulatedHours)
    double y;        // Cena w momencie przecięcia
    bool isGolden;   // true = Golden Cross, false = Death Cross
};

inline std::vector<double> calculateSMA(const std::vector<double>& prices, size_t period) {
    std::vector<double> sma(prices.size(), 0.0);
    if (prices.size() < period) return sma;

    double sum = 0.0;
    for (size_t i = 0; i < prices.size(); ++i) {
        sum += prices[i];
        if (i >= period) {
            sum -= prices[i - period];
        }
        if (i >= period - 1) {
            sma[i] = sum / period;
        } else {
            sma[i] = prices[i];
        }
    }
    return sma;
}

inline std::vector<CrossPoint> findCrossPoints(
    const std::vector<double>& timeHistory,
    const std::vector<double>& smaFast,
    const std::vector<double>& smaSlow,
    size_t minPeriod) 
{
    std::vector<CrossPoint> crosses;
    size_t limit = std::min({timeHistory.size(), smaFast.size(), smaSlow.size()});
    
    if (limit < minPeriod) 
        return crosses;

    for (size_t i = minPeriod; i < limit; ++i) {
        if (smaFast[i - 1] <= smaSlow[i - 1] && smaFast[i] > smaSlow[i]) {
            crosses.push_back({ timeHistory[i], smaFast[i], true });
        }
        else if (smaFast[i - 1] >= smaSlow[i - 1] && smaFast[i] < smaSlow[i]) {
            crosses.push_back({ timeHistory[i], smaFast[i], false });
        }
    }
    return crosses;
}