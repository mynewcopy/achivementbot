#include "BoardManager.h"
#include "Bot.h"
#include "Database.h"
#include "PartyManager.h"
#include "StreakManager.h"
#include "TaskManager.h"

#include <cstdlib>
#include <exception>
#include <iostream>

int main() {
    const char* token = std::getenv("BOT_TOKEN");
    if (!token) {
        std::cerr << "Set BOT_TOKEN environment variable\n";
        return 1;
    }

    try {
        Database db("achievementbot.db");
        if (!db.init()) {
            std::cerr << "Failed to initialize database\n";
            return 1;
        }

        PartyManager partyManager(db);
        BoardManager boardManager(db);
        TaskManager taskManager(db);
        StreakManager streakManager(db);
        Bot bot(token, partyManager, boardManager, taskManager, streakManager, db);
        bot.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
