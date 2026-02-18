#pragma once

#include <optional>
#include <sqlite3.h>
#include <string>
#include <vector>

struct User {
    long long telegramId{};
    std::string username;
    std::string registeredAt;
};

struct Party {
    int id{};
    long long ownerId{};
    std::string createdAt;
    int streakCurrent{};
    int streakThreshold{};
};

struct Board {
    int id{};
    int partyId{};
    std::string name;
    std::string createdAt;
};

struct Task {
    int id{};
    int boardId{};
    std::string text;
    bool isTemporary{};
    long long creatorId{};
    std::string taskDate;
    std::string scheduledTime;
};

struct DaySnapshot {
    std::string date;
    double completionRate{};
    int streakAfter{};
};

class Database {
public:
    explicit Database(const std::string& dbPath);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool init();

    bool upsertUser(long long telegramId, const std::string& username);
    std::optional<User> getUserByUsername(const std::string& username);
    std::optional<User> getUserById(long long telegramId);

    std::optional<int> getActivePartyIdByUser(long long telegramId);
    bool createParty(long long ownerId, int* outPartyId);
    bool addUserToParty(int partyId, long long telegramId);
    bool isUserInParty(long long telegramId, int partyId);
    std::vector<User> getPartyMembers(int partyId);
    std::optional<Party> getParty(int partyId);
    bool setPartyThreshold(int partyId, int thresholdPercent);
    bool updatePartyStreak(int partyId, int streak);

    bool createBoard(int partyId, const std::string& boardName);
    bool renameBoard(int boardId, const std::string& newName);
    std::optional<Board> getBoardByName(const std::string& boardName);
    std::vector<Board> getBoardsByParty(int partyId);

    bool createTask(int boardId,
                    const std::string& text,
                    bool isTemporary,
                    long long creatorId,
                    const std::string& taskDate,
                    const std::string& scheduledTime);
    std::vector<Task> getTasksForBoardDate(int boardId, const std::string& date);
    bool setTaskCompletion(long long telegramId, int taskId, const std::string& date, bool done);
    bool getTaskCompletion(long long telegramId, int taskId, const std::string& date);
    int countCompletedTasksForUserBoardDate(long long telegramId, int boardId, const std::string& date);

    bool archiveOldTemporaryTasks(const std::string& beforeDate);

    bool upsertDaySummary(int partyId, const std::string& date, double completionRate, int streakAfter);
    std::vector<DaySnapshot> getRecentSummaries(int partyId, int days);

    std::string nowIso() const;

private:
    sqlite3* db_{nullptr};

    bool exec(const std::string& sql);
};
