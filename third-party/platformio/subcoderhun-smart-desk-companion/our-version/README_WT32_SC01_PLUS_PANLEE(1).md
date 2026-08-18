# WT32-SC01-PLUS Panlee — Smart Desk Companion port

## Быстрый запуск Panlee 16 MB

Рабочая папка локальной версии:

```text
C:\Users\CHUWI\Documents\GitHub\WT32-SC01-PLUS-Panlee
```

Открывать проект лучше именно этой папкой, чтобы VS Code / PlatformIO не подхватывал `platformio.ini` из исходного SubCoderHUN-проекта.

PowerShell:

```powershell
cd C:\Users\CHUWI\Documents\GitHub\WT32-SC01-PLUS-Panlee

$PIO = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"

& $PIO project config
& $PIO pkg install
& $PIO run -t clean
& $PIO run
```

Успешно проверенная сборка:

```text
RAM:   78.3% (256712 / 327680)
Flash: 44.0% (2885925 / 6553600)
[SUCCESS]
```

Прошивка:

```powershell
& $PIO run -t upload --upload-port COM10
```

Порт `COM10` заменить на фактический порт платы.

Монитор:

```powershell
& $PIO device monitor -b 115200
```

### Что обязательно отличается от upstream для 16 MB Panlee

Файл:

```text
platformio.ini
```

Критические строки:

```ini
platform = platformio/espressif32@6.5.0
board = esp32-s3-devkitc-1
framework = arduino

board_build.partitions = default_16MB.csv
board_build.flash_size = 16MB
board_upload.flash_size = 16MB
board_build.arduino.memory_type = qio_qspi
```

В `build_flags`:

```ini
build_flags =
    -DBOARD_HAS_PSRAM
    -mfix-esp32-psram-cache-issue
    -I lib
    -D PLUS=1
    -D LV_MEM_SIZE="(96U * 1024U)"
```

Если LovyanGFX 1.1.7 лежит локально:

```text
lib\LovyanGFX\
```

то строку

```ini
lovyan03/LovyanGFX@1.1.7
```

из `lib_deps` не использовать. На чистой установке PlatformIO Registry у нас вернул `UnknownPackageError`, поэтому рабочая проверенная схема — vendored `lib\LovyanGFX`.

Второй обязательный файл:

```text
lib\lv_conf.h
```

Должно быть:

```c
#define LV_COLOR_16_SWAP 0
```

При `1` SquareLine-export останавливает сборку сообщением:

```text
LV_COLOR_16_SWAP should be 0 to match SquareLine Studio's settings
```

### Проверка, что PlatformIO реально видит 16 MB профиль

```powershell
& $PIO project config |
    Select-String "partitions|flash_size|memory_type|Lovyan"
```

Ожидается:

```text
board_build.partitions          = default_16MB.csv
board_build.flash_size          = 16MB
board_upload.flash_size         = 16MB
board_build.arduino.memory_type = qio_qspi
```

Название базовой платы всё ещё может содержать `ESP32-S3-DevKitC-1-N8`; это имя board definition. Для проверки нашей конфигурации ориентироваться на `project config`, размер app partition и runtime-проверку flash/PSRAM.

### Runtime-проверка физической памяти

Полезно временно добавить в `setup()`:

```cpp
Serial.printf("Flash: %u\n", ESP.getFlashChipSize());
Serial.printf("PSRAM found: %s\n", psramFound() ? "YES" : "NO");
Serial.printf("PSRAM total: %u\n", ESP.getPsramSize());
Serial.printf("PSRAM free: %u\n", ESP.getFreePsram());
```

Ожидаемый физический профиль этой Panlee:

```text
Flash ≈ 16777216 bytes
PSRAM ≈ 2097152 bytes
```

16 MB Flash увеличивает доступное пространство программы/OTA, но не увеличивает внутреннюю SRAM. Для глюков UI/Wi-Fi/Radio отдельно контролировать PSRAM и LVGL draw buffer.

---


Адаптация проекта **SubCoderHUN/WT32-SC01-PLUS** для физически проверенной платы Panlee WT32-SC01-PLUS.

## Текущий статус

Проверено на реальном устройстве:

- сборка PlatformIO — PASS;
- загрузка и загрузочный старт — PASS;
- LCD / LVGL — PHYSICAL PASS;
- touch — PHYSICAL PASS;
- UI navigation — PHYSICAL PASS;
- регулировка яркости — PHYSICAL PASS;
- Wi-Fi — PHYSICAL PASS;
- OpenWeatherMap weather path — PHYSICAL PASS;
- город **Moscow** отображается и погода обновляется;
- online radio — PHYSICAL PASS, поток реально воспроизводится;
- профиль 16 MB Flash собирается и загружается;
- текущая сборка: `2885925 / 6553600` bytes Flash (44.0%);
- внутренняя RAM при линковке: `256712 / 327680` bytes (78.3%).

Открытые мелкие вопросы:

- время для Moscow отстаёт примерно на 1 час — вероятно, остался исходный центральноевропейский timezone/DST;
- PSRAM настроена в профиле сборки, но её реальное использование приложением ещё стоит подтвердить runtime-логом;
- список городов и радиостанций в исходной архитектуре жёстко задан.

---

## Что представляет собой исходный проект

Исходный проект превращает WT32-SC01-PLUS в настольный сетевой терминал:

- часы;
- текущая погода OpenWeatherMap;
- Wi-Fi setup;
- online radio;
- управление яркостью;
- расписание dimming;
- сохранение настроек в EEPROM;
- опциональный SD-log;
- UI на LVGL;
- LovyanGFX как display/touch backend;
- UI-проект SquareLine Studio.

Главный плюс проекта — это не отдельный demo, а уже связанное приложение, где одновременно работают display, touch, Wi-Fi, weather, storage и I2S audio.

---

## Лицензия и что можно использовать

### Исходный проект

Репозиторий `SubCoderHUN/WT32-SC01-PLUS` опубликован под **Apache License 2.0**.

Это разрешительная лицензия. В практическом смысле можно:

- использовать код;
- копировать код;
- изменять код;
- делать производные версии;
- распространять исходники и бинарные сборки;
- использовать в коммерческом проекте.

При распространении производной версии необходимо как минимум:

1. сохранить копию Apache-2.0 LICENSE;
2. сохранить относящиеся к коду copyright / attribution notices;
3. явно отметить изменённые файлы;
4. если в распространяемом наборе есть NOTICE — сохранить требуемые NOTICE-атрибуции.

Рекомендуемый attribution для нашей версии:

```text
Based on SubCoderHUN/WT32-SC01-PLUS.
Original project licensed under Apache License 2.0.
Modified for Panlee WT32-SC01-PLUS / ZX3D50CE08S-V15-USRC.
```

### Важный нюанс: зависимости имеют собственные лицензии

Apache-2.0 у верхнего проекта не отменяет лицензии библиотек.

Ключевые зависимости:

| Компонент | Лицензия / статус | Практический смысл |
|---|---|---|
| LVGL | MIT | разрешительная, можно использовать и модифицировать с сохранением notice |
| LovyanGFX | FreeBSD для основной библиотеки; внутри есть компоненты с MIT/BSD/IPA Font License и др. | можно использовать, но при vendoring надо сохранять её LICENSE/credits и лицензии вложенных компонентов |
| ESP32-audioI2S | GPL-3.0 | сильный copyleft; при распространении бинарника с этой библиотекой лицензионные обязательства существенны |
| SquareLine Studio | лицензия инструмента зависит от плана | Personal предназначен не для коммерческого использования; для коммерческого редактирования/экспорта нужен подходящий план |
| OpenWeatherMap | API/service terms | нужен собственный API key; чужой ключ из опубликованного исходника не использовать |

### Самый важный лицензионный риск

`ESP32-audioI2S` распространяется под **GPL-3.0**.

Для нашего исследовательского/open-source проекта это не проблема, если мы соблюдаем GPL. Но если когда-либо понадобится закрытая коммерческая прошивка, audio layer надо считать отдельным license gate: либо сохранять GPL-совместимую модель распространения, либо заменить аудиобиблиотеку на компонент с подходящей лицензией, либо отдельно получить юридическую оценку.

### Графика, шрифты, изображения и 3D-файлы

Не надо автоматически считать любой сторонний asset безопасным только потому, что верхний репозиторий Apache-2.0.

Перед коммерческим использованием отдельно проверить provenance для:

- fonts;
- icons;
- weather graphics;
- imported images;
- 3D models;
- assets, пришедших из сторонних библиотек.

Для исследовательской копии лучше сохранять upstream attribution и не удалять оригинальные license/credit files.

---

## Что НЕ надо копировать в нашу версию

Не нужно переносить build cache и автоматически скачанные библиотеки:

```text
.pio/
.pio/libdeps/
.pio/build/
```

Это генерируемые файлы PlatformIO.

Для нашей версии предпочтительно хранить:

```text
platformio.ini
src/
include/
lib/                  # только если библиотеку сознательно vendored
README.md
LICENSE
```

---

# Главные файлы проекта

## `platformio.ini`

Управляет:

- PlatformIO platform version;
- Arduino core;
- flash size;
- partition table;
- PSRAM memory type;
- dependencies;
- build flags;
- monitor/upload settings.

Для нашей Panlee-версии рабочая основа:

```ini
platform = platformio/espressif32@6.5.0
board = esp32-s3-devkitc-1
framework = arduino

board_build.partitions = default_16MB.csv
board_build.flash_size = 16MB
board_upload.flash_size = 16MB
board_build.arduino.memory_type = qio_qspi
```

Почему зафиксирован `espressif32@6.5.0`:

неприкреплённый `platform = espressif32` подтянул современный Arduino core 3.x, после чего старый LovyanGFX 1.1.7 перестал собираться из-за удаления/изменения старых LEDC/GPIO API. С Arduino-ESP32 2.0.14 проект собирается.

---

## `lib/lv_conf.h`

Критический параметр для SquareLine UI:

```c
#define LV_COLOR_16_SWAP 0
```

Если здесь `1`, `src/ui.c` останавливает компиляцию:

```text
LV_COLOR_16_SWAP should be 0 to match SquareLine Studio's settings
```

Не менять этот параметр без одновременной проверки всей цепочки LVGL → LovyanGFX → ST7796.

---

## `src/main.cpp`

Здесь находится значительная часть glue logic:

- конфигурация LovyanGFX;
- ST7796 / Parallel8;
- touch;
- brightness;
- LVGL draw buffer;
- вызовы Wi-Fi / Time / Weather / Radio managers;
- восстановление EEPROM;
- обработка Location dropdown;
- запуск UI;
- SD enable flag.

Особенно важные точки:

```cpp
bool SDCARD_INSERTED = false;
```

и логика Location/EEPROM.

Выбранный город записывается в EEPROM по адресу `120`, после чего при наличии сети вызывается `InitWeather()`.

В исходнике также жёстко восстанавливались индексы:

```text
Rackeve       -> 0
Budapest      -> 1
Kiskunlachaza -> 2
```

В нашей версии первый город заменён на:

```text
Moscow -> 0
```

### Ограничение длины города

В исходнике есть:

```cpp
char location[15];
```

Это значит, что безопасная длина — максимум 14 символов плюс `\0`.

Если хотим нормальный произвольный город, лучше увеличить, например:

```cpp
char location[48];
```

---

# Как менять города

Город нельзя менять только в одном месте: есть UI-список и есть логика восстановления из EEPROM.

Сначала найти все связанные строки:

```powershell
Get-ChildItem .\src -Recurse -File -Include *.c,*.cpp,*.h |
    Select-String -Pattern "Rackeve|Budapest|Kiskunlachaza|Moscow|ui_LocationDropdown"
```

Нужно проверить две группы мест.

## 1. Dropdown UI

Список вариантов создаётся экспортированным SquareLine/LVGL кодом.

В зависимости от версии export он может находиться в:

```text
src/ui.c
src/screens/
src/components/
```

Меняем текст options, например:

```text
Moscow
Budapest
Kiskunlachaza
```

или создаём свой список.

## 2. `src/main.cpp`

Restore logic должна соответствовать тем же индексам:

```cpp
if (location.indexOf("Moscow") == 0) {
    lv_dropdown_set_selected(ui_LocationDropdown, 0);
    lv_label_set_text(ui_locationtext, location.c_str());
}
```

Если добавляется четвёртый город, нужен соответствующий index `3`.

После изменения порядка городов лучше ещё раз выбрать город через UI, чтобы EEPROM содержала актуальное значение.

## Лучше, чем жёсткий список

Следующая нормальная архитектура:

```text
City name
Latitude
Longitude
Timezone
Country code
```

и хранение профиля в Preferences/NVS вместо жёсткого dropdown.

Для OpenWeatherMap ещё надёжнее использовать latitude/longitude, чем только название города.

---

# Moscow и время

Погода Moscow уже работает.

Текущее время примерно на один час позади локального московского времени. Это хорошо согласуется с тем, что исходный проект ориентирован на Венгрию и мог использовать центральноевропейский summer offset.

Искать timezone logic здесь:

```text
src/Managers/TimeManager/time_manager.cpp
src/Managers/TimeManager/time_manager.h
```

Команда поиска:

```powershell
Get-ChildItem .\src\Managers\TimeManager -Recurse -File |
    Select-String -Pattern "configTime|configTzTime|gmt|offset|daylight|3600|7200"
```

Для Moscow логика должна соответствовать:

```text
UTC+3
DST = 0
```

Если код использует секунды:

```text
gmt offset = 10800
daylight offset = 0
```

Лучше хранить timezone отдельно от weather location.

---

# Weather

Основные файлы:

```text
src/Features/weather/weather.cpp
src/Features/weather/weather.h
```

Здесь находятся:

- OpenWeatherMap request;
- parsing ответа;
- температура;
- humidity;
- weather condition;
- icon selection;
- обновление UI.

## API key

В upstream-исходнике API key был встроен прямо в код.

В нашей версии так делать не надо.

Предпочтительный вариант:

```text
include/weather_secrets.h
```

с локальным содержимым:

```cpp
#pragma once
#define OWM_API_KEY "YOUR_OWN_KEY"
```

и добавить в `.gitignore`:

```text
include/weather_secrets.h
```

В репозиторий можно положить:

```text
include/weather_secrets.example.h
```

без реального ключа.

---

# Как менять радиостанции

Основные файлы:

```text
src/Features/radio/radio.cpp
src/Features/radio/radio.h
```

`src/main.cpp` вызывает:

```cpp
SetupRadio();
ReadRadFromEEPROM();
```

Поэтому список station names, stream URLs и обработка выбора должны быть в radio module.

Сначала показать все URL и места выбора:

```powershell
Get-ChildItem .\src\Features\radio -Recurse -File |
    Select-String -Pattern "http://|https://|connecttohost|station|dropdown|SetupRadio"
```

Обычно для замены станции надо изменить две связанные вещи:

```text
display name
stream URL
```

Не менять только label без URL и не менять только URL, если station index используется в `switch`.

После изменения порядка списка сохранённый EEPROM index может указывать уже на другую станцию. Поэтому после перепрошивки лучше вручную выбрать станцию снова.

## Какие потоки подходят

Аудиотракт использует ESP32-audioI2S.

Для embedded radio удобнее прямые stream URL:

```text
MP3
AAC/AACP
ICY/Shoutcast/Icecast
поддерживаемые playlist URL
```

Надёжнее сначала проверить URL в VLC, а затем на ESP32.

Некоторые HTTPS/HLS/redirect streams могут работать хуже старого простого MP3/AAC Icecast endpoint.

## Хорошая следующая архитектура

Вместо hardcoded `switch`:

```cpp
struct RadioStation {
    const char *name;
    const char *url;
};
```

и единый массив:

```cpp
static const RadioStation stations[] = {
    {"Station 1", "https://..."},
    {"Station 2", "https://..."},
};
```

Тогда dropdown генерируется автоматически из массива, и добавление станции не требует редактировать несколько switch/case.

---

# I2S audio

Исходный проект использует:

```text
BCLK -> GPIO 36
LRC  -> GPIO 35
DOUT -> GPIO 37
```

На нашей физической плате online radio реально воспроизводится, поэтому application-level I2S path подтверждён.

Это один из самых интересных элементов upstream-проекта, потому что он показывает полный путь:

```text
Wi-Fi stream
    ↓
decoder
    ↓
I2S
    ↓
on-board / external audio path
```

---

# Wi-Fi

Основные файлы:

```text
src/Managers/WiFiManager/
```

Отвечают за:

- подключение;
- сохранённые credentials;
- reconnect;
- UI state;
- network availability для weather/radio.

Wi-Fi credentials в текущей архитектуре сохраняются через EEPROM manager.

---

# EEPROM / persistent settings

Основные файлы:

```text
src/Managers/EEPROMManager/
```

Что уже хранится:

- SSID;
- Wi-Fi password;
- selected location;
- brightness;
- dimming interval;
- radio selection.

Перед изменением layout EEPROM полезно составить отдельную таблицу адресов.

Нельзя бездумно менять длину одной строки, если соседние значения используют фиксированные offsets.

Для дальнейшей версии лучше перейти на:

```text
Preferences / NVS
```

с именованными keys.

---

# SD card

Основные файлы:

```text
src/Managers/SDManager/
```

В `src/main.cpp`:

```cpp
bool SDCARD_INSERTED = false;
```

При `true` проект может создавать `log.txt`.

Включать только после проверки конкретной SD-карты и pinout.

---

# SquareLine Studio / UI

В репозитории есть исходный SquareLine project:

```text
WT32-SC01-PLUS.spj
WT32-SC01-PLUS.sll
```

И экспортированные файлы:

```text
src/ui.c
src/ui.h
src/ui_events.h
src/ui_helpers.c
src/ui_helpers.h

src/screens/
src/components/
src/images/
```

## Важно

Эти файлы в значительной степени generated.

Если изменить их вручную, а потом снова экспортировать UI из SquareLine, ручные изменения могут быть потеряны.

Поэтому:

- application logic держать в `main.cpp`, `Features/`, `Managers/`;
- внешний вид менять в `.spj` и экспортировать;
- после export проверять diff перед заменой UI-файлов.

### Что можно заменять целиком после export

Обычно безопаснее заменять export bundle:

```text
src/screens/
src/components/
src/images/
src/ui.c
src/ui.h
src/ui_helpers.*
src/ui_events.*
```

но не затрагивать:

```text
src/main.cpp
src/Features/
src/Managers/
```

без отдельного merge.

---

# LovyanGFX

В нашей локальной Panlee-версии LovyanGFX 1.1.7 временно vendored:

```text
lib/LovyanGFX/
```

Причина — PlatformIO Registry перестал находить старую запись:

```text
lovyan03/LovyanGFX @ 1.1.7
```

при новой чистой установке.

Если библиотека vendored, её собственные LICENSE/credits нельзя удалять.

Не надо копировать только отдельные `.cpp/.hpp` без license metadata — лучше хранить библиотеку целиком или подключать фиксированный upstream commit/tag.

---

# Память и возможная оптимизация

Даже после перехода на 16 MB Flash линкер показывает:

```text
RAM: 78.3%
```

Это внутренняя RAM ESP32-S3, а не Flash.

В upstream `src/main.cpp` LVGL использует крупный статический draw buffer:

```cpp
static lv_color_t buf[screenWidth * 100];
```

При `480` пикселях и RGB565 это около:

```text
480 × 100 × 2 = 96 000 bytes
```

Это один из главных кандидатов на оптимизацию.

Варианты:

1. уменьшить число строк draw buffer;
2. проверить реальные требования DMA;
3. убедиться, что PSRAM обнаружена;
4. переносить только те allocations, которые безопасно держать в external RAM.

Не переносить display/DMA buffers в PSRAM вслепую.

---

# Runtime memory test

Полезно временно добавить:

```cpp
Serial.printf("Flash: %u\n", ESP.getFlashChipSize());
Serial.printf("PSRAM found: %s\n", psramFound() ? "YES" : "NO");
Serial.printf("PSRAM total: %u\n", ESP.getPsramSize());
Serial.printf("PSRAM free: %u\n", ESP.getFreePsram());
Serial.printf("Heap free: %u\n", ESP.getFreeHeap());
```

Для нашей платы ожидается физический профиль:

```text
Flash ≈ 16 MB
PSRAM ≈ 2 MB
```

---

# Что из upstream особенно полезно забрать

Наиболее ценные reusable части:

1. **полный LVGL application flow**, а не отдельный display demo;
2. **LovyanGFX конфигурация ST7796 + Parallel8 + touch**;
3. **Wi-Fi UI + reconnect logic**;
4. **OpenWeatherMap integration**;
5. **EEPROM persistence**;
6. **brightness + scheduled dimming**;
7. **online radio UI и ESP32-audioI2S integration**;
8. **SD logging**;
9. **SquareLine project как редактируемый источник UI**;
10. **3D case / auxiliary design materials** — после отдельной проверки provenance/licensing assets.

---

# Рекомендуемая структура нашей производной версии

```text
WT32-SC01-PLUS-Panlee/
├── LICENSE
├── README.md
├── THIRD_PARTY_NOTICES.md
├── platformio.ini
├── include/
│   ├── weather_secrets.example.h
│   └── ...
├── lib/
│   └── LovyanGFX/             # если остаётся vendored
├── src/
│   ├── main.cpp
│   ├── Features/
│   │   ├── radio/
│   │   └── weather/
│   ├── Managers/
│   │   ├── EEPROMManager/
│   │   ├── SDManager/
│   │   ├── TimeManager/
│   │   └── WiFiManager/
│   ├── screens/
│   ├── components/
│   ├── images/
│   └── ui.*
└── docs/
    ├── HARDWARE_VALIDATION.md
    ├── LICENSE_AUDIT.md
    └── CUSTOMIZATION.md
```

---

# Следующие разумные улучшения

Без изменения общей идеи приложения:

1. timezone Moscow = UTC+3 without DST;
2. произвольный город или lat/lon вместо трёх hardcoded вариантов;
3. station table вместо hardcoded radio switch;
4. API key вне исходника;
5. Preferences/NVS вместо фиксированных EEPROM offsets;
6. runtime PSRAM diagnostics;
7. уменьшение внутреннего LVGL buffer;
8. отдельный `THIRD_PARTY_NOTICES.md`;
9. исключить `.pio/` из version control;
10. сохранить upstream attribution и Apache-2.0 LICENSE.

---

## Provenance

This derivative is based on:

```text
SubCoderHUN/WT32-SC01-PLUS
Original license: Apache-2.0
```

Our changes are focused on:

```text
Panlee WT32-SC01-PLUS
16 MB Flash profile
2 MB PSRAM-capable profile
reproducible PlatformIO toolchain
Moscow weather location
physical hardware validation
```

This README is engineering documentation, not legal advice. For commercial distribution, re-check the exact dependency versions and all third-party asset licenses before release.
