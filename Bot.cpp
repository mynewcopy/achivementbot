#include "Bot.h"

#include <curl/curl.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <sstream>
#include <thread>
#include <vector>

namespace {

size_t writeCb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}

std::string urlEncode(const std::string& text) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return text;
    }
    char* output = curl_easy_escape(curl, text.c_str(), static_cast<int>(text.size()));
    std::string encoded = output ? output : "";
    if (output) {
        curl_free(output);
    }
    curl_easy_cleanup(curl);
    return encoded;
}

std::vector<std::string> split(const std::string& input) {
    std::istringstream ss(input);
    std::vector<std::string> out;
    std::string token;
    while (ss >> token) {
        out.push_back(token);
    }
    return out;
}

std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \n\r\t");
    if (b == std::string::npos) {
        return "";
    }
    const auto e = s.find_last_not_of(" \n\r\t");
    return s.substr(b, e - b + 1);
}

} // namespace

Bot::Bot(std::string token,
         PartyManager& partyManager,
         BoardManager& boardManager,
         TaskManager& taskManager,
         StreakManager& streakManager,
         Database& db)
    : token_(std::move(token)),
      partyManager_(partyManager),
      boardManager_(boardManager),
      taskManager_(taskManager),
      streakManager_(streakManager),
      db_(db) {}

std::string Bot::apiCall(const std::string& method, const std::string& payload) const {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return "";
    }
    std::string response;
    const std::string url = "https://api.telegram.org/bot" + token_ + "/" + method;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return response;
}

void Bot::sendMessage(long long chatId, const std::string& text) const {
    const std::string payload = "chat_id=" + std::to_string(chatId) + "&text=" + urlEncode(text);
    apiCall("sendMessage", payload);
}

std::string Bot::handleCommand(long long chatId,
                               bool isPrivate,
                               long long userId,
                               const std::string& username,
                               const std::string& text,
                               const std::string& botUsername) {
    const std::string regErr = partyManager_.ensureRegistered(userId, username);
    if (!regErr.empty()) {
        return regErr;
    }

    auto tokens = split(text);
    if (tokens.empty()) {
        return "";
    }
    const std::string cmd = tokens[0];

    if (cmd == "/start") {
        return "Вы зарегистрированы. Используйте /create_party, /create_board, /status <board>.";
    }
    if (!isPrivate && cmd == "/create_party") {
        return "Создание пати разрешено только в личном чате.";
    }
    if (cmd == "/create_party") {
        return partyManager_.createParty(userId);
    }
    if (!isPrivate && (cmd == "/add_friends" || cmd == "/create_board" || cmd == "/rename_board" ||
                       cmd == "/add_task" || cmd == "/add_temp" || cmd == "/done" || cmd == "/undo" ||
                       cmd == "/set_threshold")) {
        return "Эта команда доступна только в личном чате.";
    }
    if (cmd == "/add_friends") {
        tokens.erase(tokens.begin());
        return partyManager_.addMembersByUsernames(userId, tokens);
    }
    if (cmd == "/set_threshold") {
        if (tokens.size() != 2) {
            return "Использование: /set_threshold <1..100>";
        }
        return partyManager_.setThreshold(userId, std::stoi(tokens[1]));
    }
    if (cmd == "/create_board") {
        if (tokens.size() != 2) {
            return "Использование: /create_board board_name";
        }
        return boardManager_.createBoard(userId, tokens[1]);
    }
    if (cmd == "/rename_board") {
        if (tokens.size() != 3) {
            return "Использование: /rename_board old_name new_name";
        }
        return boardManager_.renameBoard(userId, tokens[1], tokens[2]);
    }
    if (cmd == "/add_task") {
        if (tokens.size() < 3) {
            return "Использование: /add_task board_name текст задачи";
        }
        const std::string boardName = tokens[1];
        const std::string taskText = trim(text.substr(text.find(boardName) + boardName.size()));
        return taskManager_.addTask(userId, boardName, taskText);
    }
    if (cmd == "/add_temp") {
        if (tokens.size() < 4) {
            return "Использование: /add_temp board_name HH:MM текст";
        }
        const std::string boardName = tokens[1];
        const std::string time = tokens[2];
        const auto pos = text.find(time);
        const std::string payload = pos == std::string::npos ? "" : trim(text.substr(pos + time.size()));
        return taskManager_.addTemporaryTask(userId, boardName, time, payload);
    }
    if (cmd == "/done" || cmd == "/undo") {
        if (tokens.size() != 2) {
            return "Использование: /done <task_id> или /undo <task_id>";
        }
        return taskManager_.markTask(userId, std::stoi(tokens[1]), cmd == "/done");
    }

    std::string boardName;
    if (cmd == "/status" || cmd == "/board") {
        if (tokens.size() != 2) {
            return "Использование: /status board_name";
        }
        boardName = tokens[1];
    } else if (cmd.size() > 1 && cmd[0] == '/' && cmd.find_first_of("_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") == 1) {
        boardName = cmd.substr(1);
    } else if (cmd.rfind("@" + botUsername, 0) == 0 && tokens.size() >= 2) {
        boardName = tokens[1];
    } else {
        return "Неизвестная команда.";
    }

    const auto board = boardManager_.resolveAccessibleBoard(userId, boardName);
    if (!board.has_value()) {
        return "Доска не найдена или нет доступа.";
    }

    const std::string date = db_.nowIso();
    db_.archiveOldTemporaryTasks(date);
    streakManager_.rollupDay(board->partyId, date);
    const auto status = streakManager_.buildStatusForUser(userId, *board, date);

    std::ostringstream out;
    out << "📋 Доска: " << status.boardName << "\n"
        << "📅 Дата: " << status.date << "\n"
        << "\nЗадачи:\n"
        << status.taskSection << "\n"
        << "Лично выполнено: " << status.completed << "/" << status.total << "\n"
        << "Общий прогресс пати: " << std::fixed << std::setprecision(1) << status.partyCompletionPercent << "%\n"
        << "Текущая серия: " << status.streak << "\n"
        << "\nИстория:\n"
        << status.historySection;
    return out.str();
}

void Bot::processUpdate(const std::string& updateJson) {
    static const std::regex updRe(R"("update_id"\s*:\s*(\d+).+?"message"\s*:\s*\{(.+?)\}\s*\})");
    static const std::regex chatIdRe(R"re("chat"\s*:\s*\{[^\}]*"id"\s*:\s*(-?\d+)[^\}]*"type"\s*:\s*"([^"]+)")re");
    static const std::regex userRe(R"re("from"\s*:\s*\{[^\}]*"id"\s*:\s*(\d+)[^\}]*"username"\s*:\s*"([^"]*)")re");
    static const std::regex textRe(R"re("text"\s*:\s*"([^"]*)")re");

    std::smatch match;
    std::string::const_iterator searchStart(updateJson.cbegin());
    const std::string botUsername = std::getenv("BOT_USERNAME") ? std::getenv("BOT_USERNAME") : "";

    while (std::regex_search(searchStart, updateJson.cend(), match, updRe)) {
        const long long updateId = std::stoll(match[1]);
        std::string messageBlob = match[2];

        std::smatch c, u, t;
        if (!std::regex_search(messageBlob, c, chatIdRe) || !std::regex_search(messageBlob, u, userRe) ||
            !std::regex_search(messageBlob, t, textRe)) {
            searchStart = match.suffix().first;
            continue;
        }

        const long long chatId = std::stoll(c[1]);
        const bool isPrivate = c[2] == "private";
        const long long userId = std::stoll(u[1]);
        const std::string username = u[2];
        std::string text = t[1];
        std::replace(text.begin(), text.end(), '\\', ' ');

        const std::string reply = handleCommand(chatId, isPrivate, userId, username, text, botUsername);
        if (!reply.empty()) {
            sendMessage(chatId, reply);
        }
        offset_ = (std::max)(offset_, updateId + 1);
        searchStart = match.suffix().first;
    }
}

void Bot::run() {
    std::cout << "Bot polling started...\n";
    while (true) {
        const std::string payload = "timeout=30&offset=" + std::to_string(offset_);
        const std::string response = apiCall("getUpdates", payload);
        if (!response.empty()) {
            processUpdate(response);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }
}
