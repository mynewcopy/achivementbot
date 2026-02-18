#pragma once

#include "Database.h"

#include <string>

struct PartyStatusView {
    std::string boardName;
    std::string date;
    std::string taskSection;
    int completed{};
    int total{};
    double partyCompletionPercent{};
    int streak{};
    std::string historySection;
};

class StreakManager {
public:
    explicit StreakManager(Database& db);

    bool rollupDay(int partyId, const std::string& date);
    PartyStatusView buildStatusForUser(long long requesterId, const Board& board, const std::string& date);

private:
    Database& db_;
};
