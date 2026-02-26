#include "TaskManager.h"

#include <regex>

TaskManager::TaskManager(Database& db) : db_(db) {}

std::string TaskManager::addTask(long long requesterId, const std::string& boardName, const std::string& text) {
    const auto board = db_.getBoardByName(boardName);
    if (!board.has_value()) {
        return "Доска не найдена.";
    }
    if (!db_.isUserInParty(requesterId, board->partyId)) {
        return "Нет доступа к доске.";
    }
    if (!db_.createTask(board->id, text, false, requesterId, db_.nowIso(), "")) {
        return "Не удалось создать задачу.";
    }
    return "Задача добавлена.";
}

std::string TaskManager::addTemporaryTask(long long requesterId,
                                          const std::string& boardName,
                                          const std::string& hhmm,
                                          const std::string& text) {
    static const std::regex timePattern(R"(^([01]\d|2[0-3]):[0-5]\d$)");
    if (!std::regex_match(hhmm, timePattern)) {
        return "Время должно быть в формате HH:MM.";
    }
    const auto board = db_.getBoardByName(boardName);
    if (!board.has_value()) {
        return "Доска не найдена.";
    }
    if (!db_.isUserInParty(requesterId, board->partyId)) {
        return "Нет доступа к доске.";
    }
    if (!db_.createTask(board->id, text, true, requesterId, db_.nowIso(), hhmm)) {
        return "Не удалось добавить временную задачу.";
    }
    return "Временная задача добавлена на " + hhmm;
}

std::string TaskManager::markTask(long long requesterId, int taskId, bool done) {
    if (!db_.setTaskCompletion(requesterId, taskId, db_.nowIso(), done)) {
        return "Не удалось обновить отметку.";
    }
    return done ? "Задача отмечена как выполненная." : "Отметка выполнения снята.";
}
