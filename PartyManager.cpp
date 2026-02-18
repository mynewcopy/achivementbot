#include "PartyManager.h"

#include <sstream>

PartyManager::PartyManager(Database& db) : db_(db) {}

std::string PartyManager::ensureRegistered(long long telegramId, const std::string& username) {
    if (username.empty()) {
        return "У вас не установлен username в Telegram. Установите @username и повторите.";
    }
    if (!db_.upsertUser(telegramId, username)) {
        return "Не удалось зарегистрировать пользователя.";
    }
    return "";
}

std::string PartyManager::createParty(long long ownerId) {
    if (db_.getActivePartyIdByUser(ownerId).has_value()) {
        return "Вы уже состоите в пати. Доступна только одна активная пати.";
    }
    int partyId = 0;
    if (!db_.createParty(ownerId, &partyId)) {
        return "Ошибка создания пати.";
    }
    if (!db_.addUserToParty(partyId, ownerId)) {
        return "Пати создана, но не удалось добавить владельца.";
    }
    return "Пати создана. ID: " + std::to_string(partyId);
}

std::string PartyManager::addMembersByUsernames(long long requesterId, const std::vector<std::string>& usernames) {
    const auto partyId = db_.getActivePartyIdByUser(requesterId);
    if (!partyId.has_value()) {
        return "Сначала создайте пати через /create_party.";
    }
    std::ostringstream report;
    for (const auto& u : usernames) {
        const auto clean = (!u.empty() && u[0] == '@') ? u.substr(1) : u;
        const auto user = db_.getUserByUsername(clean);
        if (!user.has_value()) {
            report << "❌ @" << clean << ": пользователь не зарегистрирован у бота\n";
            continue;
        }
        if (db_.getActivePartyIdByUser(user->telegramId).has_value()) {
            report << "❌ @" << clean << ": уже состоит в другой пати\n";
            continue;
        }
        if (!db_.addUserToParty(*partyId, user->telegramId)) {
            report << "❌ @" << clean << ": не удалось добавить\n";
            continue;
        }
        report << "✅ @" << clean << " добавлен(а)\n";
    }
    return report.str().empty() ? "Нет username для добавления." : report.str();
}

std::optional<int> PartyManager::getUserParty(long long telegramId) {
    return db_.getActivePartyIdByUser(telegramId);
}

bool PartyManager::isPartyOwner(long long telegramId, int partyId) {
    const auto p = db_.getParty(partyId);
    return p.has_value() && p->ownerId == telegramId;
}

std::string PartyManager::setThreshold(long long requesterId, int thresholdPercent) {
    if (thresholdPercent < 1 || thresholdPercent > 100) {
        return "Порог должен быть в диапазоне 1..100.";
    }
    const auto partyId = db_.getActivePartyIdByUser(requesterId);
    if (!partyId.has_value()) {
        return "Вы не состоите в пати.";
    }
    if (!isPartyOwner(requesterId, *partyId)) {
        return "Только владелец пати может менять порог серии.";
    }
    if (!db_.setPartyThreshold(*partyId, thresholdPercent)) {
        return "Не удалось обновить порог.";
    }
    return "Порог серии обновлён: " + std::to_string(thresholdPercent) + "%";
}
