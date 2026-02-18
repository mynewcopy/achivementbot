# Achievement Party Telegram Bot (C++)

Telegram-бот для команды друзей с пати-логикой, досками задач и общей серией.

## Возможности
- Регистрация пользователя при первом сообщении в ЛС.
- Создание одной активной пати и добавление друзей по `@username`.
- Доски задач (глобально уникальные имена, формат `[A-Za-z0-9_]+`).
- Ежедневные задачи + временные задачи с временем `HH:MM`.
- Отметки выполнения только в личном чате.
- Агрегированный `/status <board>` в одном сообщении:
  - задачи с отметками текущего пользователя,
  - локальная статистика,
  - общий процент пати,
  - текущая серия,
  - краткая история последних дней.
- В группе доступен только режим просмотра статуса.

## Команды
### Личный чат
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

### Групповой чат
- `/status board_name`
- `/board board_name`
- `/board_name` (shortcut)
- `@BotUsername board_name`

## Сборка
```bash
cmake -S . -B build
cmake --build build -j
```

## Запуск
```bash
export BOT_TOKEN="<telegram_bot_token>"
export BOT_USERNAME="<bot_username_without_@>"
./build/achivementbot
```

База данных SQLite создается в `achievementbot.db`.
