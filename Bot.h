#pragma once

#include "BoardManager.h"
#include "PartyManager.h"
#include "StreakManager.h"
#include "TaskManager.h"

#include <string>

class Bot {
public:
    Bot(std::string token,
        PartyManager& partyManager,
        BoardManager& boardManager,
        TaskManager& taskManager,
        StreakManager& streakManager,
        Database& db);

    void run();

private:
    std::string token_;
    PartyManager& partyManager_;
    BoardManager& boardManager_;
    TaskManager& taskManager_;
    StreakManager& streakManager_;
    Database& db_;
    long long offset_{0};

    void processUpdate(const std::string& updateJson);
    std::string handleCommand(long long chatId,
                              bool isPrivate,
                              long long userId,
                              const std::string& username,
                              const std::string& text,
                              const std::string& botUsername);

    std::string apiCall(const std::string& method, const std::string& payload) const;
    void sendMessage(long long chatId, const std::string& text) const;
};
