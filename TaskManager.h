#pragma once

#include "Database.h"

#include <string>

class TaskManager {
public:
    explicit TaskManager(Database& db);

    std::string addTask(long long requesterId, const std::string& boardName, const std::string& text);
    std::string addTemporaryTask(long long requesterId,
                                 const std::string& boardName,
                                 const std::string& hhmm,
                                 const std::string& text);
    std::string markTask(long long requesterId, int taskId, bool done);

private:
    Database& db_;
};
