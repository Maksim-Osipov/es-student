# es-student

Практические задания курса «Встраиваемые системы», Raspberry Pi Pico / RP2040.

**Занятие 3.1 готово к работе в вузе:** [прошивка, подключение и запуск всех измерений](311-adc/README.md).
Готовый файл для BOOTSEL доступен в [комплекте сборки](https://github.com/Maksim-Osipov/es-student/actions/workflows/adc-firmware.yml)
и локально в `311-adc/firmware/311_adc.uf2`.
Исходные результаты 3.1 помечены как учебная модель без платы;
`python capture_all.py` из папки `311-adc` заменяет их реальными измерениями.

Логи `device-*.log` для 1.3 и модуля 2 — **учебные примеры без платы**.
Их источник указан в шапке; физические измерения они не подтверждают.
Исходные скрипты `check-*.py` сохранены для получения логов по USB.

| Занятие | Проект | Что реализовано |
| --- | --- | --- |
| 1.3 | `131-hello-usb`, `133-led-button-usb`, `134-led-module` | Добавлены недостающие примеры логов |
| 2.1.1–2.1.6 | `211-command-usb` | Строки, таблица команд, карта памяти, адреса объектов, структура паспорта, таблица векторов и GPIO |
| 2.2.1–2.2.5 | `221-command-time` | Частоты, неблокирующее мигание, uptime, блокирующий расчёт π, профилирование, смена частоты ядра |
| 2.3.1–2.3.8 | `231-firmware-template` | Протокол, API, независимые задачи LED/кнопки/π, пять секундомеров, библиотеки и отделение SDK |
| 2.3.9 | `239-two-buttons` | Независимые кнопки GP15 и GP14; состояния LED и круг периодов 250 → 500 → 1000 мс |
| 3.1.1–3.1.5 | `311-adc` | АЦП GP26, температура, телеметрия, CSV/графики и управление RC-цепью с оценкой τ |

Светодиод подключён к GP25, кнопки замыкают свои выводы на GND. Вторая кнопка нужна только проекту `239-two-buttons`.
Дополнительное задание 2.3.9д в эту реализацию не входит.

## Сборка

Требуются Pico SDK 2.3.0, Arm GNU Toolchain, CMake, Make и picotool.
Переменная `PICO_SDK_PATH` должна указывать на каталог SDK.
Пример для любого проекта (подставить его папку):

```sh
cmake -S 231-firmware-template -B 231-firmware-template/build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build 231-firmware-template/build --parallel
```

В Windows при необходимости передайте путь к Make через `-DCMAKE_MAKE_PROGRAM=...`.
Файлы `.uf2`, `.elf`, `.bin` и `.map` появляются в `build/`.
Для отключения секундомеров поменяйте `PROFILING_ENABLED=1` на `PROFILING_ENABLED=0` в CMake проекта 2.3.

## Проверка без платы

```sh
python -m pip install -r 311-adc/requirements.txt
cmake -S tests/host -B tests/host/build
cmake --build tests/host/build --config Debug
ctest --test-dir tests/host/build -C Debug --output-on-failure
```

Тесты исполняют C-код прошивки с управляемыми часами и GPIO: проверяют сетку периода,
дребезг, удержание и независимость кнопок, число π, переполнение числовых аргументов,
отказы без изменения состояния, Backspace, неполный кадр и сброс профилирования.
На Windows используется установленный Visual Studio, на Linux — обычный C-компилятор.

Учебные логи воспроизводятся после сборки проектов 2.1, 2.2 и тестового приложения:

```sh
python tools/generate-example-logs.py --runtime tests/host/build/example_runtime
```

Для Visual Studio путь к приложению: `tests/host/build/Debug/example_runtime.exe`.
В примерах 2.1 адреса, инструкции и размеры извлекаются из ELF/bin; адреса стека/кучи моделируются.
В 2.2 время вычисления оценивается по 585 тактам на член ряда, частоты задаются моделью.
В 2.3 ответы выдаёт настоящий C-код протокола/API/задач с модельным временем и GPIO.
Нулевой серийный номер во всех примерах — условный идентификатор.

Примеры 3.1 воспроизводятся отдельно: `python tools/generate-adc-examples.py --runtime tests/host/build/Debug/adc_runtime.exe`
(на Linux — `tests/host/build/adc_runtime`). Они исполняют настоящий C-код `311-adc` с
моделью АЦП, времени и RC-цепи. Источник указан в логах, JSON рядом с CSV и на графиках.

## Проверка на плате

1. Подключить Pico с нажатой BOOTSEL и скопировать нужный `.uf2` на диск RPI-RP2.
2. Установить `pyserial`: `python -m pip install pyserial`.
3. Перейти в каталог проекта, закрыть терминалы, занимающие COM-порт, и запустить его `check-*.py`.
4. Для 1.3.3 нажимать кнопку по указаниям скрипта; для 2.3.8 начать с перезапуска платы и кнопку не трогать.
5. Полученные скриптами логи заменяют учебные примеры. Отправить изменения в GitHub.

## Где менять поведение шаблона

- Перенос LED или смена электрической полярности кнопки: `platform/platform.c`.
- Действие кнопки: функция, передаваемая в `button_task_init()` из `main.c`.
- Новая команда: `logic/api/api-callbacks.c`.
- Новая задача: её модуль в `logic/`, инициализация и вызов из `main.c`, исходник в CMake.
- Общие библиотеки: `libs/`. Их заголовки не открывают SDK логике.

Официальные проверки вызываются из `.github/workflows/` по образцам курса.

## GitHub Actions

[![check-1-1-1](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-1-1.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-1-1.yml)
[![check-1-1-2](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-1-2.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-1-2.yml)
[![check-1-1-3](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-1-3.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-1-3.yml)
[![check-1-1-4](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-1-4.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-1-4.yml)
[![check-1-1-5](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-1-5.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-1-5.yml)
[![check-1-2-1](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-2-1.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-2-1.yml)
[![check-1-2-2](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-2-2.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-2-2.yml)
[![check-1-2-3](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-2-3.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-2-3.yml)
[![check-1-3-1](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-3-1.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-3-1.yml)
[![check-1-3-2](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-3-2.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-3-2.yml)
[![check-1-3-3](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-3-3.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-3-3.yml)
[![check-1-3-4](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-3-4.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-3-4.yml)
[![check-1-3-5](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-3-5.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-3-5.yml)
[![check-1-3-6](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-3-6.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-3-6.yml)
[![check-1-3-7](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-3-7.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-1-3-7.yml)
[![check-2-1-1](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-1-1.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-1-1.yml)
[![check-2-1-2](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-1-2.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-1-2.yml)
[![check-2-1-3](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-1-3.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-1-3.yml)
[![check-2-1-4](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-1-4.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-1-4.yml)
[![check-2-1-5](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-1-5.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-1-5.yml)
[![check-2-1-6](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-1-6.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-1-6.yml)
[![check-2-2-1](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-2-1.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-2-1.yml)
[![check-2-2-2](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-2-2.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-2-2.yml)
[![check-2-2-3](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-2-3.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-2-3.yml)
[![check-2-2-4](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-2-4.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-2-4.yml)
[![check-2-2-5](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-2-5.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-2-5.yml)
[![check-2-3-1](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-3-1.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-3-1.yml)
[![check-2-3-2](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-3-2.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-3-2.yml)
[![check-2-3-3](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-3-3.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-3-3.yml)
[![check-2-3-4](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-3-4.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-3-4.yml)
[![check-2-3-5](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-3-5.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-3-5.yml)
[![check-2-3-6](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-3-6.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-3-6.yml)
[![check-2-3-7](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-3-7.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-3-7.yml)
[![check-2-3-8](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-3-8.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-3-8.yml)
[![check-2-3-9](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-3-9.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-2-3-9.yml)
[![host-tests](https://github.com/Maksim-Osipov/es-student/actions/workflows/host-tests.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/host-tests.yml)
[![check-3-1-1](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-3-1-1.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-3-1-1.yml)
[![check-3-1-2](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-3-1-2.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-3-1-2.yml)
[![check-3-1-3](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-3-1-3.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-3-1-3.yml)
[![check-3-1-4](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-3-1-4.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-3-1-4.yml)
[![check-3-1-5](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-3-1-5.yml/badge.svg)](https://github.com/Maksim-Osipov/es-student/actions/workflows/check-3-1-5.yml)
