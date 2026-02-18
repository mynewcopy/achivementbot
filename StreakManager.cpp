#include "StreakManager.h"

#include <iomanip>
#include <sstream>

StreakManager::StreakManager(Database& db) : db_(db) {}

bool StreakManager::rollupDay(int partyId, const std::string& date) {
    const auto party = db_.getParty(partyId);
    if (!party.has_value()) {
        return false;
    }
    const auto members = db_.getPartyMembers(partyId);
    const auto boards = db_.getBoardsByParty(partyId);
    if (members.empty() || boards.empty()) {
        return db_.upsertDaySummary(partyId, date, 0.0, party->streakCurrent);
    }

    bool allPassedThreshold = true;
    double sumMemberPercent = 0.0;

    int totalTasksAcrossBoards = 0;
    for (const auto& board : boards) {
        totalTasksAcrossBoards += static_cast<int>(db_.getTasksForBoardDate(board.id, date).size());
    }

    if (totalTasksAcrossBoards == 0) {
        allPassedThreshold = false;
    }

    for (const auto& member : members) {
        int done = 0;
        for (const auto& board : boards) {
            done += db_.countCompletedTasksForUserBoardDate(member.telegramId, board.id, date);
        }
        const double percent = totalTasksAcrossBoards == 0 ? 0.0 : (100.0 * done / totalTasksAcrossBoards);
        sumMemberPercent += percent;
        if (percent + 1e-9 < party->streakThreshold) {
            allPassedThreshold = false;
        }
    }

    int newStreak = allPassedThreshold ? party->streakCurrent + 1 : 0;
    if (!db_.updatePartyStreak(partyId, newStreak)) {
        return false;
    }

    const double partyCompletion = members.empty() ? 0.0 : sumMemberPercent / members.size();
    return db_.upsertDaySummary(partyId, date, partyCompletion, newStreak);
}

PartyStatusView StreakManager::buildStatusForUser(long long requesterId,
                                                  const Board& board,
                                                  const std::string& date) {
    PartyStatusView view;
    view.boardName = board.name;
    view.date = date;

    const auto tasks = db_.getTasksForBoardDate(board.id, date);
    std::ostringstream tasksSection;
    int completed = 0;
    int idx = 1;
    for (const auto& task : tasks) {
        const bool done = db_.getTaskCompletion(requesterId, task.id, date);
        completed += done ? 1 : 0;
        tasksSection << (done ? "✅ " : "⬜ ") << idx++ << ". " << task.text;
        if (task.isTemporary) {
            tasksSection << " [временная " << task.scheduledTime << "]";
        }
        tasksSection << " (#" << task.id << ")\n";
    }
    if (tasks.empty()) {
        tasksSection << "— На сегодня задач нет.\n";
    }
    view.taskSection = tasksSection.str();
    view.completed = completed;
    view.total = static_cast<int>(tasks.size());

    const auto party = db_.getParty(board.partyId);
    const auto members = db_.getPartyMembers(board.partyId);
    double partyPercent = 0.0;
    if (!members.empty() && view.total > 0) {
        double sumPercent = 0.0;
        for (const auto& member : members) {
            const int doneCount = db_.countCompletedTasksForUserBoardDate(member.telegramId, board.id, date);
            sumPercent += (100.0 * doneCount / view.total);
        }
        partyPercent = sumPercent / members.size();
    }
    view.partyCompletionPercent = partyPercent;
    view.streak = party ? party->streakCurrent : 0;

    const auto history = db_.getRecentSummaries(board.partyId, 5);
    std::ostringstream historyText;
    if (history.empty()) {
        historyText << "— История пока пуста.";
    } else {
        for (const auto& row : history) {
            historyText << row.date << ": " << std::fixed << std::setprecision(1) << row.completionRate
                        << "% | серия=" << row.streakAfter << "\n";
        }
    }
    view.historySection = historyText.str();
    return view;
}
