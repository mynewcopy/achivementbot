#include "Database.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {

class Statement {
public:
    Statement(sqlite3* db, const std::string& sql) {
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db));
        }
    }
    ~Statement() {
        if (stmt_) {
            sqlite3_finalize(stmt_);
        }
    }
    sqlite3_stmt* get() { return stmt_; }

private:
    sqlite3_stmt* stmt_{nullptr};
};

} // namespace

Database::Database(const std::string& dbPath) {
    if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool Database::exec(const std::string& sql) {
    char* err = nullptr;
    const int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        if (err) {
            std::cerr << "SQLite error: " << err << '\n';
            sqlite3_free(err);
        }
        return false;
    }
    return true;
}

bool Database::init() {
    return exec(
        "PRAGMA foreign_keys = ON;"
        "CREATE TABLE IF NOT EXISTS users ("
        "telegram_id INTEGER PRIMARY KEY,"
        "username TEXT UNIQUE NOT NULL,"
        "registered_at TEXT NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS parties ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "owner_id INTEGER NOT NULL,"
        "created_at TEXT NOT NULL,"
        "streak_current INTEGER NOT NULL DEFAULT 0,"
        "streak_threshold INTEGER NOT NULL DEFAULT 80,"
        "FOREIGN KEY(owner_id) REFERENCES users(telegram_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS party_members ("
        "party_id INTEGER NOT NULL,"
        "telegram_id INTEGER NOT NULL UNIQUE,"
        "joined_at TEXT NOT NULL,"
        "PRIMARY KEY(party_id, telegram_id),"
        "FOREIGN KEY(party_id) REFERENCES parties(id) ON DELETE CASCADE,"
        "FOREIGN KEY(telegram_id) REFERENCES users(telegram_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS boards ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "party_id INTEGER NOT NULL,"
        "name TEXT NOT NULL UNIQUE,"
        "created_at TEXT NOT NULL,"
        "FOREIGN KEY(party_id) REFERENCES parties(id) ON DELETE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS tasks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "board_id INTEGER NOT NULL,"
        "text TEXT NOT NULL,"
        "is_temporary INTEGER NOT NULL DEFAULT 0,"
        "creator_id INTEGER NOT NULL,"
        "task_date TEXT NOT NULL,"
        "scheduled_time TEXT,"
        "archived INTEGER NOT NULL DEFAULT 0,"
        "FOREIGN KEY(board_id) REFERENCES boards(id) ON DELETE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS task_completions ("
        "telegram_id INTEGER NOT NULL,"
        "task_id INTEGER NOT NULL,"
        "completion_date TEXT NOT NULL,"
        "done INTEGER NOT NULL,"
        "PRIMARY KEY(telegram_id, task_id, completion_date),"
        "FOREIGN KEY(task_id) REFERENCES tasks(id) ON DELETE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS party_day_summary ("
        "party_id INTEGER NOT NULL,"
        "summary_date TEXT NOT NULL,"
        "completion_rate REAL NOT NULL,"
        "streak_after INTEGER NOT NULL,"
        "PRIMARY KEY(party_id, summary_date),"
        "FOREIGN KEY(party_id) REFERENCES parties(id) ON DELETE CASCADE"
        ");");
}

std::string Database::nowIso() const {
    const auto now = std::chrono::system_clock::now();
    const auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    gmtime_r(&t, &tm);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d");
    return ss.str();
}

bool Database::upsertUser(long long telegramId, const std::string& username) {
    Statement st(db_,
                 "INSERT INTO users(telegram_id, username, registered_at) VALUES(?, ?, ?) "
                 "ON CONFLICT(telegram_id) DO UPDATE SET username = excluded.username");
    sqlite3_bind_int64(st.get(), 1, telegramId);
    sqlite3_bind_text(st.get(), 2, username.c_str(), -1, SQLITE_TRANSIENT);
    const auto now = nowIso();
    sqlite3_bind_text(st.get(), 3, now.c_str(), -1, SQLITE_TRANSIENT);
    return sqlite3_step(st.get()) == SQLITE_DONE;
}

std::optional<User> Database::getUserByUsername(const std::string& username) {
    Statement st(db_, "SELECT telegram_id, username, registered_at FROM users WHERE username = ?");
    sqlite3_bind_text(st.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(st.get()) == SQLITE_ROW) {
        return User{sqlite3_column_int64(st.get(), 0),
                    reinterpret_cast<const char*>(sqlite3_column_text(st.get(), 1)),
                    reinterpret_cast<const char*>(sqlite3_column_text(st.get(), 2))};
    }
    return std::nullopt;
}

std::optional<User> Database::getUserById(long long telegramId) {
    Statement st(db_, "SELECT telegram_id, username, registered_at FROM users WHERE telegram_id = ?");
    sqlite3_bind_int64(st.get(), 1, telegramId);
    if (sqlite3_step(st.get()) == SQLITE_ROW) {
        return User{sqlite3_column_int64(st.get(), 0),
                    reinterpret_cast<const char*>(sqlite3_column_text(st.get(), 1)),
                    reinterpret_cast<const char*>(sqlite3_column_text(st.get(), 2))};
    }
    return std::nullopt;
}

std::optional<int> Database::getActivePartyIdByUser(long long telegramId) {
    Statement st(db_, "SELECT party_id FROM party_members WHERE telegram_id = ?");
    sqlite3_bind_int64(st.get(), 1, telegramId);
    if (sqlite3_step(st.get()) == SQLITE_ROW) {
        return sqlite3_column_int(st.get(), 0);
    }
    return std::nullopt;
}

bool Database::createParty(long long ownerId, int* outPartyId) {
    Statement st(db_, "INSERT INTO parties(owner_id, created_at) VALUES(?, ?)");
    sqlite3_bind_int64(st.get(), 1, ownerId);
    const auto now = nowIso();
    sqlite3_bind_text(st.get(), 2, now.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        return false;
    }
    *outPartyId = static_cast<int>(sqlite3_last_insert_rowid(db_));
    return true;
}

bool Database::addUserToParty(int partyId, long long telegramId) {
    Statement st(db_, "INSERT INTO party_members(party_id, telegram_id, joined_at) VALUES(?, ?, ?)");
    sqlite3_bind_int(st.get(), 1, partyId);
    sqlite3_bind_int64(st.get(), 2, telegramId);
    const auto now = nowIso();
    sqlite3_bind_text(st.get(), 3, now.c_str(), -1, SQLITE_TRANSIENT);
    return sqlite3_step(st.get()) == SQLITE_DONE;
}

bool Database::isUserInParty(long long telegramId, int partyId) {
    Statement st(db_, "SELECT 1 FROM party_members WHERE party_id = ? AND telegram_id = ?");
    sqlite3_bind_int(st.get(), 1, partyId);
    sqlite3_bind_int64(st.get(), 2, telegramId);
    return sqlite3_step(st.get()) == SQLITE_ROW;
}

std::vector<User> Database::getPartyMembers(int partyId) {
    Statement st(db_,
                 "SELECT u.telegram_id, u.username, u.registered_at "
                 "FROM users u JOIN party_members pm ON pm.telegram_id = u.telegram_id "
                 "WHERE pm.party_id = ? ORDER BY u.username");
    sqlite3_bind_int(st.get(), 1, partyId);
    std::vector<User> users;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        users.push_back(User{sqlite3_column_int64(st.get(), 0),
                             reinterpret_cast<const char*>(sqlite3_column_text(st.get(), 1)),
                             reinterpret_cast<const char*>(sqlite3_column_text(st.get(), 2))});
    }
    return users;
}

std::optional<Party> Database::getParty(int partyId) {
    Statement st(db_,
                 "SELECT id, owner_id, created_at, streak_current, streak_threshold FROM parties WHERE id = ?");
    sqlite3_bind_int(st.get(), 1, partyId);
    if (sqlite3_step(st.get()) == SQLITE_ROW) {
        return Party{sqlite3_column_int(st.get(), 0),
                     sqlite3_column_int64(st.get(), 1),
                     reinterpret_cast<const char*>(sqlite3_column_text(st.get(), 2)),
                     sqlite3_column_int(st.get(), 3),
                     sqlite3_column_int(st.get(), 4)};
    }
    return std::nullopt;
}

bool Database::setPartyThreshold(int partyId, int thresholdPercent) {
    Statement st(db_, "UPDATE parties SET streak_threshold = ? WHERE id = ?");
    sqlite3_bind_int(st.get(), 1, thresholdPercent);
    sqlite3_bind_int(st.get(), 2, partyId);
    return sqlite3_step(st.get()) == SQLITE_DONE;
}

bool Database::updatePartyStreak(int partyId, int streak) {
    Statement st(db_, "UPDATE parties SET streak_current = ? WHERE id = ?");
    sqlite3_bind_int(st.get(), 1, streak);
    sqlite3_bind_int(st.get(), 2, partyId);
    return sqlite3_step(st.get()) == SQLITE_DONE;
}

bool Database::createBoard(int partyId, const std::string& boardName) {
    Statement st(db_, "INSERT INTO boards(party_id, name, created_at) VALUES(?, ?, ?)");
    sqlite3_bind_int(st.get(), 1, partyId);
    sqlite3_bind_text(st.get(), 2, boardName.c_str(), -1, SQLITE_TRANSIENT);
    const auto now = nowIso();
    sqlite3_bind_text(st.get(), 3, now.c_str(), -1, SQLITE_TRANSIENT);
    return sqlite3_step(st.get()) == SQLITE_DONE;
}

bool Database::renameBoard(int boardId, const std::string& newName) {
    Statement st(db_, "UPDATE boards SET name = ? WHERE id = ?");
    sqlite3_bind_text(st.get(), 1, newName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(st.get(), 2, boardId);
    return sqlite3_step(st.get()) == SQLITE_DONE;
}

std::optional<Board> Database::getBoardByName(const std::string& boardName) {
    Statement st(db_, "SELECT id, party_id, name, created_at FROM boards WHERE name = ?");
    sqlite3_bind_text(st.get(), 1, boardName.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(st.get()) == SQLITE_ROW) {
        return Board{sqlite3_column_int(st.get(), 0),
                     sqlite3_column_int(st.get(), 1),
                     reinterpret_cast<const char*>(sqlite3_column_text(st.get(), 2)),
                     reinterpret_cast<const char*>(sqlite3_column_text(st.get(), 3))};
    }
    return std::nullopt;
}

std::vector<Board> Database::getBoardsByParty(int partyId) {
    Statement st(db_, "SELECT id, party_id, name, created_at FROM boards WHERE party_id = ? ORDER BY id");
    sqlite3_bind_int(st.get(), 1, partyId);
    std::vector<Board> boards;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        boards.push_back(Board{sqlite3_column_int(st.get(), 0),
                               sqlite3_column_int(st.get(), 1),
                               reinterpret_cast<const char*>(sqlite3_column_text(st.get(), 2)),
                               reinterpret_cast<const char*>(sqlite3_column_text(st.get(), 3))});
    }
    return boards;
}

bool Database::createTask(int boardId,
                          const std::string& text,
                          bool isTemporary,
                          long long creatorId,
                          const std::string& taskDate,
                          const std::string& scheduledTime) {
    Statement st(db_,
                 "INSERT INTO tasks(board_id, text, is_temporary, creator_id, task_date, scheduled_time) "
                 "VALUES(?, ?, ?, ?, ?, ?)");
    sqlite3_bind_int(st.get(), 1, boardId);
    sqlite3_bind_text(st.get(), 2, text.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(st.get(), 3, isTemporary ? 1 : 0);
    sqlite3_bind_int64(st.get(), 4, creatorId);
    sqlite3_bind_text(st.get(), 5, taskDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 6, scheduledTime.c_str(), -1, SQLITE_TRANSIENT);
    return sqlite3_step(st.get()) == SQLITE_DONE;
}

std::vector<Task> Database::getTasksForBoardDate(int boardId, const std::string& date) {
    Statement st(db_,
                 "SELECT id, board_id, text, is_temporary, creator_id, task_date, IFNULL(scheduled_time, '') "
                 "FROM tasks WHERE board_id = ? AND task_date = ? AND archived = 0 ORDER BY id");
    sqlite3_bind_int(st.get(), 1, boardId);
    sqlite3_bind_text(st.get(), 2, date.c_str(), -1, SQLITE_TRANSIENT);
    std::vector<Task> tasks;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        tasks.push_back(Task{sqlite3_column_int(st.get(), 0),
                             sqlite3_column_int(st.get(), 1),
                             reinterpret_cast<const char*>(sqlite3_column_text(st.get(), 2)),
                             sqlite3_column_int(st.get(), 3) == 1,
                             sqlite3_column_int64(st.get(), 4),
                             reinterpret_cast<const char*>(sqlite3_column_text(st.get(), 5)),
                             reinterpret_cast<const char*>(sqlite3_column_text(st.get(), 6))});
    }
    return tasks;
}

bool Database::setTaskCompletion(long long telegramId, int taskId, const std::string& date, bool done) {
    Statement st(db_,
                 "INSERT INTO task_completions(telegram_id, task_id, completion_date, done) VALUES(?, ?, ?, ?) "
                 "ON CONFLICT(telegram_id, task_id, completion_date) DO UPDATE SET done = excluded.done");
    sqlite3_bind_int64(st.get(), 1, telegramId);
    sqlite3_bind_int(st.get(), 2, taskId);
    sqlite3_bind_text(st.get(), 3, date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(st.get(), 4, done ? 1 : 0);
    return sqlite3_step(st.get()) == SQLITE_DONE;
}

bool Database::getTaskCompletion(long long telegramId, int taskId, const std::string& date) {
    Statement st(db_,
                 "SELECT done FROM task_completions WHERE telegram_id = ? AND task_id = ? AND completion_date = ?");
    sqlite3_bind_int64(st.get(), 1, telegramId);
    sqlite3_bind_int(st.get(), 2, taskId);
    sqlite3_bind_text(st.get(), 3, date.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(st.get()) == SQLITE_ROW) {
        return sqlite3_column_int(st.get(), 0) == 1;
    }
    return false;
}

int Database::countCompletedTasksForUserBoardDate(long long telegramId, int boardId, const std::string& date) {
    Statement st(db_,
                 "SELECT COUNT(*) FROM task_completions tc "
                 "JOIN tasks t ON t.id = tc.task_id "
                 "WHERE tc.telegram_id = ? AND t.board_id = ? AND tc.completion_date = ? AND tc.done = 1");
    sqlite3_bind_int64(st.get(), 1, telegramId);
    sqlite3_bind_int(st.get(), 2, boardId);
    sqlite3_bind_text(st.get(), 3, date.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(st.get()) == SQLITE_ROW) {
        return sqlite3_column_int(st.get(), 0);
    }
    return 0;
}

bool Database::archiveOldTemporaryTasks(const std::string& beforeDate) {
    Statement st(db_,
                 "UPDATE tasks SET archived = 1 WHERE is_temporary = 1 AND task_date < ? AND archived = 0");
    sqlite3_bind_text(st.get(), 1, beforeDate.c_str(), -1, SQLITE_TRANSIENT);
    return sqlite3_step(st.get()) == SQLITE_DONE;
}

bool Database::upsertDaySummary(int partyId, const std::string& date, double completionRate, int streakAfter) {
    Statement st(db_,
                 "INSERT INTO party_day_summary(party_id, summary_date, completion_rate, streak_after) "
                 "VALUES(?, ?, ?, ?) "
                 "ON CONFLICT(party_id, summary_date) DO UPDATE SET "
                 "completion_rate = excluded.completion_rate, streak_after = excluded.streak_after");
    sqlite3_bind_int(st.get(), 1, partyId);
    sqlite3_bind_text(st.get(), 2, date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(st.get(), 3, completionRate);
    sqlite3_bind_int(st.get(), 4, streakAfter);
    return sqlite3_step(st.get()) == SQLITE_DONE;
}

std::vector<DaySnapshot> Database::getRecentSummaries(int partyId, int days) {
    Statement st(db_,
                 "SELECT summary_date, completion_rate, streak_after "
                 "FROM party_day_summary WHERE party_id = ? ORDER BY summary_date DESC LIMIT ?");
    sqlite3_bind_int(st.get(), 1, partyId);
    sqlite3_bind_int(st.get(), 2, days);
    std::vector<DaySnapshot> snapshots;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        snapshots.push_back(DaySnapshot{reinterpret_cast<const char*>(sqlite3_column_text(st.get(), 0)),
                                        sqlite3_column_double(st.get(), 1),
                                        sqlite3_column_int(st.get(), 2)});
    }
    return snapshots;
}
