# Achievement Party Telegram Bot (C++)

Telegram-бот для команды друзей с пати-логикой, досками задач и общей серией.

## Что взять из Telegram (обязательно)

### 1) Получить токен через BotFather
1. Откройте в Telegram `@BotFather`.
2. Команда `/newbot`.
3. Задайте имя и `username`.
4. Скопируйте токен вида:
   `1234567890:AAHxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx`

Это значение для переменной `BOT_TOKEN`.

### 2) Взять username бота без `@`
Если бот `@my_party_bot`, то:
- `BOT_USERNAME=my_party_bot`

---

## ПОЛНЫЙ ГАЙД ДЛЯ WINDOWS (VS Code)

Ниже — путь «с нуля» именно для Windows, если вы уже открыли папку проекта в VS Code.

### Шаг 0. Что должно быть установлено

Установите (если ещё не стоит):
1. **Visual Studio Code**
2. **Git for Windows**
3. **CMake** (добавить в PATH)
4. **Visual Studio 2022 Build Tools**
   - компонент `Desktop development with C++`
   - MSVC toolchain + Windows SDK
5. **vcpkg** (для библиотек `sqlite3` и `curl`)

> Альтернатива: можно собирать через MinGW, но для этого проекта проще и стабильнее через MSVC + vcpkg.

### Шаг 1. Откройте правильный терминал в VS Code

В VS Code:
- `Terminal` → `New Terminal`
- лучше выбрать **PowerShell**

Проверьте инструменты:
```powershell
cmake --version
git --version
```

### Шаг 2. Установите зависимости через vcpkg

Если vcpkg уже установлен, пропустите клонирование.

```powershell
git clone https://github.com/microsoft/vcpkg Q:\PROGRAMER33\zv\vcpkg
Q:\PROGRAMER33\zv\vcpkg\bootstrap-vcpkg.bat
Q:\PROGRAMER33\zv\vcpkg\vcpkg install sqlite3:x64-windows curl:x64-windows
```

### Шаг 3. Сконфигурируйте и соберите проект

Из корня проекта (там где `CMakeLists.txt`):


Если ваш `vcpkg` лежит в другом месте (как у вас `Q:\PROGRAMER33\zv`), просто подставляйте свой путь в `CMAKE_TOOLCHAIN_FILE`.

Рекомендуемый вариант через переменную:
```powershell
$env:VCPKG_ROOT="Q:/PROGRAMER33/zv/vcpkg"
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build --config Release
```

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=Q:/PROGRAMER33/zv/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build --config Release
```


> ⚠️ В Windows PowerShell 5.1 оператор `&&` не поддерживается.
> Запускайте команды по одной строке (или используйте `;`).

Пример для PowerShell:
```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=Q:/PROGRAMER33/zv/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build --config Release
```

Пример для `cmd.exe`:
```bat
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=Q:/PROGRAMER33/zv/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build --config Release
```

После сборки бинарник обычно здесь:
- `build\Release\achievementbot.exe`

### Шаг 4. Вставьте токен и username (это главный момент)

Вы писали: «куда что вставлять». На Windows это делается перед запуском в том же терминале:

```powershell
$env:BOT_TOKEN="1234567890:AAHxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
$env:BOT_USERNAME="my_party_bot"
```

- `BOT_TOKEN` = строка от BotFather
- `BOT_USERNAME` = username бота **без `@`**

Проверка:
```powershell
echo $env:BOT_TOKEN
echo $env:BOT_USERNAME
```

### Шаг 5. Запустите бота

```powershell
.\build\Release\achievementbot.exe
```

Если всё ок — бот начнёт long polling (`getUpdates`).

### Шаг 6. Первый запуск в Telegram (обязательно)

1. В личке боту отправьте `/start`.
2. Создайте пати: `/create_party`.
3. Друзья тоже должны написать боту `/start`.
4. Добавьте их: `/add_friends @friend1 @friend2`.
5. Создайте доску: `/create_board daily_board`.
6. Добавьте задачи: `/add_task daily_board Почистить почту`.
7. Отметьте задачу: `/done <task_id>`.
8. Посмотрите статус: `/status daily_board`.

---

## Если хотите запуск одной кнопкой в VS Code

Создайте `.vscode/launch.json` (опционально) и добавьте env:

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Run achievementbot",
      "type": "cppvsdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build/Release/achievementbot.exe",
      "cwd": "${workspaceFolder}",
      "environment": [
        { "name": "BOT_TOKEN", "value": "<YOUR_BOT_TOKEN>" },
        { "name": "BOT_USERNAME", "value": "<YOUR_BOT_USERNAME>" }
      ]
    }
  ]
}
```

Тогда не нужно каждый раз вручную делать `$env:...`.

---

## Команды бота

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

## Частые проблемы на Windows

1. **`Set BOT_TOKEN environment variable`**
   - Не задан `BOT_TOKEN` в текущем терминале.

2. **`curl/sqlite3 not found` при CMake configure**
   - Не подключили `vcpkg` toolchain в команде `cmake -S ...`.

3. **Бот не отвечает в группе**
   - Проверьте, что используете `@BotUsername board_name` или `/status board_name`.
   - Убедитесь, что пользователь имеет доступ к доске (состоит в нужной пати).

4. **Не добавляется друг по username**
   - Друг должен сначала написать боту `/start` в личке.
   - У друга должен быть установлен Telegram `@username`.


5. **`CommandNotFoundException` при `./build/Release/achivementbot.exe`**
   - Опечатка в имени: правильно `achievementbot.exe` (есть **e** после `v`).
   - Запускайте так:
     ```powershell
     .\build\Release\achievementbot.exe
     ```
   - Если не уверены, проверьте файл:
     ```powershell
     Get-ChildItem .\build\Release
     ```

---

## Linux (кратко)

```bash
cmake -S . -B build
cmake --build build -j
export BOT_TOKEN="<ваш_токен_от_BotFather>"
export BOT_USERNAME="<username_бота_без_@>"
./build/achievementbot
```

База данных SQLite создаётся рядом с бинарником: `achievementbot.db`.
