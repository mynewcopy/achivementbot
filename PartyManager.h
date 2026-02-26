#pragma once

#include "Database.h"

#include <optional>
#include <string>
#include <vector>

class PartyManager {
public:
    explicit PartyManager(Database& db);

    std::string ensureRegistered(long long telegramId, const std::string& username);
    std::string createParty(long long ownerId);
    std::string addMembersByUsernames(long long requesterId, const std::vector<std::string>& usernames);
    std::optional<int> getUserParty(long long telegramId);
    bool isPartyOwner(long long telegramId, int partyId);
    std::string setThreshold(long long requesterId, int thresholdPercent);

private:
    Database& db_;
};
