# Achievement Party Telegram Bot (C++)

Telegram-бот для команды друзей с пати-логикой, досками задач и общей серией.

## Что именно нужно взять из Telegram и куда вставлять

### 1) Создать бота в BotFather и получить **токен**
1. В Telegram откройте `@BotFather`.
2. Выполните `/newbot` и задайте имя/username бота.
3. BotFather пришлёт строку вида:
   `1234567890:AAHxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx`

Это и есть **BOT_TOKEN**.

### 2) Взять username бота (без `@`)
Если бот называется `@my_party_bot`, то значение переменной:
- `BOT_USERNAME=my_party_bot`

### 3) Экспортировать переменные окружения перед запуском
```bash
export BOT_TOKEN="1234567890:AAHxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
export BOT_USERNAME="my_party_bot"
```

> `BOT_USERNAME` нужен для режима вызова в группе через `@BotUsername board_name`.

---

## Быстрый запуск (шаг за шагом)

### 1) Установить зависимости (Linux)
Нужны: CMake, компилятор C++17, SQLite3, libcurl.

### 2) Собрать проект
```bash
cmake -S . -B build
cmake --build build -j
```

### 3) Запустить бота
```bash
export BOT_TOKEN="<ваш_токен_от_BotFather>"
export BOT_USERNAME="<username_бота_без_@>"
./build/achivementbot
```

После запуска бот начинает long polling (`getUpdates`).

### 4) Первичная настройка в Telegram
1. Напишите боту в **личку**: `/start` (это регистрация пользователя).
2. Создайте пати: `/create_party`.
3. Добавьте друзей (они тоже должны написать боту `/start`):
   `/add_friends @friend1 @friend2`
4. Создайте доску:
   `/create_board daily_board`
5. Добавьте задачи:
   `/add_task daily_board Почистить почту`
6. Отметьте выполнение:
   `/done <task_id>`
7. Посмотреть агрегированный статус:
   `/status daily_board`

---

## В каком чате какие команды работают

### Личный чат (управление)
- `/start`
- `/create_party`
- `/add_friends @user1 @user2`
- `/set_threshold 80`
- `/create_board board_name`
- `/rename_board old_name new_name`
- `/add_task board_name текст`
- `/add_temp board_name HH:MM текст`
- `/done task_id`
- `/undo task_id`
- `/status board_name`

### Групповой чат (только просмотр)
- `/status board_name`
- `/board board_name`
- `/board_name` (shortcut)
- `@BotUsername board_name`

В группе нельзя создавать пати/доски и отмечать задачи.

---

## Полезные замечания
- База данных SQLite создаётся рядом с бинарником: `achievementbot.db`.
- Если при запуске видите `Set BOT_TOKEN environment variable`, значит вы не выставили `BOT_TOKEN`.
- Для корректной работы приглашений у участников должен быть выставлен Telegram `@username`.
