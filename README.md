# math — обработка NMEA-логов и поиск проездов у камеры

Консольное приложение для Windows, которое читает GPS-логи в формате NMEA, находит точки рядом с заданной камерой и формирует `rez.txt` со строками для `go.cmd list`.

## Состав проекта

- `main.cpp` — точка входа, основная логика.
- `progress_bar.cpp` — реализация прогресс-бара.
- `progress_bar.hpp` — заголовок прогресс-бара (нужно добавить в репозиторий, в исходных файлах отсутствует).
- `math.vcxproj` — файл проекта Visual Studio 2017 (v141, Win32).
- `input.ini` — конфигурация (пути, координаты камеры, лимиты).
- `rez.txt` — результат работы.

## Требования

- Windows
- Visual Studio 2017 (Platform Toolset v141, Win32)
- Boost 1.74.0
- nmealib
- cpp_progress_bar

## Ссылки для скачивания зависимостей

### Boost 1.74.0

Официальная страница версии:  
[https://www.boost.org/users/history/version_1_74_0.html](https://www.boost.org/users/history/version_1_74_0.html)

Готовые бинарники для Windows (MSVC 14.2, 64-bit) на SourceForge:  
[https://sourceforge.net/projects/boost/files/boost-binaries/1.74.0/](https://sourceforge.net/projects/boost/files/boost-binaries/1.74.0/)

Исходники:  
`https://boostorg.jfrog.io/artifactory/main/release/1.74.0/source/boost_1_74_0.7z`

> В `math.vcxproj` прописан путь `D:\source\repos\gps\boost_1_74_0`. После скачивания положите Boost по этому пути или отредактируйте `IncludePath` в файле проекта.

### nmealib

Используется библиотека **nmealib**.

GitHub-репозиторий:  
[https://github.com/Simon-Leung/nmealib](https://github.com/Simon-Leung/nmealib)

Оригинальный сайт:  
[http://nmea.sourceforge.net/](http://nmea.sourceforge.net/)

Библиотека предоставляет заголовки `nmea/nmea.h`, `nmea/gmath.h`, `nmea/info.h`, `nmea/parser.h` и функции `nmea_parser_init`, `nmea_parse`, `nmea_info2pos`, `nmea_distance`, `nmea_degree2radian`.

Нужно собрать под Win32 в конфигурациях **Release** и **Debug** и получить:
- `lib/nmea.lib` (Release)
- `lib/nmea_d.lib` (Debug)
- папку `include/nmea/` с заголовками

### cpp_progress_bar

Используется библиотека **karasikov/cpp_progress_bar** (класс `ProgressBar`).

GitHub-репозиторий:  
[https://github.com/karasikov/cpp_progress_bar](https://github.com/karasikov/cpp_progress_bar)

В проекте `math.vcxproj` прописан путь:

```xml
<ClCompile Include="..\cpp_progress_bar-master\progress_bar.cpp">
```

Скачайте репозиторий и положите папку `cpp_progress_bar-master` рядом с проектом, либо отредактируйте путь в `.vcxproj`.

Убедитесь, что в репозитории есть `progress_bar.hpp`, иначе добавьте его из того же репозитория.

## Структура каталогов

Проект ожидает следующую структуру (относительно `math.vcxproj`):

```text
repo/
├─ math.vcxproj
├─ main.cpp
├─ progress_bar.cpp
├─ progress_bar.hpp
├─ input.ini
├─ include/
│  └─ nmea/
│     ├─ nmea.h
│     ├─ gmath.h
│     ├─ info.h
│     └─ ...
├─ lib/
│  ├─ nmea.lib
│  └─ nmea_d.lib
├─ cpp_progress_bar-master/
│  ├─ progress_bar.cpp
│  └─ progress_bar.hpp
└─ boost_1_74_0/          (по пути из .vcxproj)
```

В `math.vcxproj` прописаны:

- `AdditionalIncludeDirectories` → `../../include`
- `AdditionalLibraryDirectories` → `../../lib`
- `IncludePath` → `D:\source\repos\gps\boost_1_74_0`
- `ClCompile Include` → `..\cpp_progress_bar-master\progress_bar.cpp`

## input.ini

Пример конфигурации:

```ini
[files]
in_path=e:\cards\gpx\
in_file_mask=Card_202*.gpx
out_file_full=rez.txt

[limits]
delta_time=10
;sec
camera_max_distance=90
;m
speed_limit=0
;km/hr

[camera]
lat=55.611836
lon=37.364985
```

- `in_path` — папка с GPS-логами.
- `in_file_mask` — маска файлов.
- `out_file_full` — имя выходного файла.
- `delta_time` — интервал в секундах.
- `camera_max_distance` — максимальное расстояние до камеры, м.
- `speed_limit` — ограничение скорости, км/ч (0 — не учитывать).
- `lat`, `lon` — координаты камеры.

## Сборка

1. Установите Visual Studio 2017 с набором инструментов v141 (Win32).
2. Скачайте Boost 1.74.0 и разместите по пути `D:\source\repos\gps\boost_1_74_0` (или измените `IncludePath` в `math.vcxproj`).
3. Скачайте и соберите **nmealib** под Win32 (Release и Debug).
4. Скачайте **cpp_progress_bar** и положите папку `cpp_progress_bar-master` рядом с проектом.
5. Разместите заголовки NMEA в `include/nmea/`, библиотеки — в `lib/`.
6. Добавьте `progress_bar.hpp` в каталог проекта.
7. Откройте `math.vcxproj` в Visual Studio.
8. Выберите конфигурацию:
   - **Release|Win32** → `math.exe`
   - **Debug|Win32** → `math_d.exe`
9. Соберите решение (**Build → Build Solution**).

Каталоги вывода:
- `../../build/Win32/Release/math/`
- `../../build/Win32/Debug/math/`

## Запуск

```cmd
math.exe
```

Приложение читает `input.ini` из текущего каталога. Результат записывается в файл, указанный в `out_file_full`.

## Результат

Строки вида:

```text
call go.cmd list Card_20230624.gpx 04:52:38 20230620045228 20230620045248 105.7 58.6 58.6m
```

Формат:

```text
call go.cmd list [имя_файла] [время] [дата_начала] [дата_конца] [скорость] [расстояние_за_сек] [расстояние_до_камеры]
```

## Что коммитить в репозиторий

Минимальный набор:

```text
math.vcxproj
main.cpp
progress_bar.cpp
progress_bar.hpp
input.ini
include/nmea/            (заголовки NMEA)
lib/nmea.lib
lib/nmea_d.lib
cpp_progress_bar-master/ (или инструкция по скачиванию)
README.md
.gitignore
```

Boost 1.74.0 лучше **не коммитить** — указать в README ссылку для скачивания или подключить как git submodule.

### Пример .gitignore

```gitignore
build/
Debug/
Release/
*.user
*.suo
*.sdf
*.opensdf
*.VC.db
*.VC.opendb
ipch/
```

## Примечания

- В `math.vcxproj` для Release выбран `CompileAsC`, хотя `main.cpp` содержит код C++ с Boost и STL. Если сборка Release падает, измените `CompileAs` на `CompileAsCpp` (или `Default`) для конфигурации **Release|Win32**.
- В Debug-конфигурации используется `nmea_d.lib`, в Release — `nmea.lib`.
- Для работы Boost `property_tree` и `lexical_cast` достаточно заголовочных файлов Boost (header-only), но установщик с бинарниками всё равно рекомендуется.
