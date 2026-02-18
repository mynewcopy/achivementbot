#pragma once

#include "Database.h"

#include <optional>
#include <regex>
#include <string>

class BoardManager {
public:
    explicit BoardManager(Database& db);

    std::string createBoard(long long requesterId, const std::string& boardName);
    std::string renameBoard(long long requesterId, const std::string& oldName, const std::string& newName);
    std::optional<Board> resolveAccessibleBoard(long long requesterId, const std::string& boardName);

private:
    Database& db_;
    std::regex boardNamePattern_{R"(^[A-Za-z0-9_]+$)"};
};
