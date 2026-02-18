#include "BoardManager.h"

BoardManager::BoardManager(Database& db) : db_(db) {}

std::string BoardManager::createBoard(long long requesterId, const std::string& boardName) {
    if (!std::regex_match(boardName, boardNamePattern_)) {
        return "Имя доски должно содержать только латиницу, цифры и _";
    }
    const auto partyId = db_.getActivePartyIdByUser(requesterId);
    if (!partyId.has_value()) {
        return "Сначала вступите в пати.";
    }
    if (!db_.createBoard(*partyId, boardName)) {
        return "Не удалось создать доску. Возможно имя уже занято.";
    }
    return "Доска создана: " + boardName;
}

std::string BoardManager::renameBoard(long long requesterId,
                                      const std::string& oldName,
                                      const std::string& newName) {
    if (!std::regex_match(newName, boardNamePattern_)) {
        return "Новое имя доски невалидно.";
    }
    const auto board = db_.getBoardByName(oldName);
    if (!board.has_value()) {
        return "Доска не найдена.";
    }
    if (!db_.isUserInParty(requesterId, board->partyId)) {
        return "Нет доступа к этой доске.";
    }
    if (!db_.renameBoard(board->id, newName)) {
        return "Переименование не удалось. Возможно имя занято.";
    }
    return "Доска переименована: " + oldName + " -> " + newName;
}

std::optional<Board> BoardManager::resolveAccessibleBoard(long long requesterId, const std::string& boardName) {
    const auto board = db_.getBoardByName(boardName);
    if (!board.has_value()) {
        return std::nullopt;
    }
    if (!db_.isUserInParty(requesterId, board->partyId)) {
        return std::nullopt;
    }
    return board;
}
