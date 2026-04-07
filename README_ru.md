
# batch-pqc

**PoC: Пакетная подпись для российских пост-квантовых криптоалгоритмов**

[![Лицензия: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![PRs Приветствуются](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](CONTRIBUTING.md)
[![Статус CI](https://github.com/cherninkiy/batch-pqc/actions/workflows/ci.yml/badge.svg)](https://github.com/cherninkiy/batch-pqc/actions/workflows/ci.yml)

**[English version](README.md)**

## Обзор


Этот проект демонстрирует **пакетную подпись** (на основе дерева Меркла) для трёх российских пост-квантовых схем подписи:

- [Шиповник](https://github.com/QAPP-tech/shipovnik_tc26) (на основе протокола Штерна)
- [Гиперикум](https://github.com/QAPP-tech/hypericum_tc26) (stateless, на основе SPHINCS+, использует хеш-функцию **Стрибог** из своей реализации)
- [Крыжовник](https://github.com/ElenaKirshanova/pqc_LWR_signature) (на основе решёток, аналог Dilithium)

Цель — измерить ускорение при подписи пакета сообщений с помощью одной подписи корня дерева + доказательств по сравнению с последовательной подписью. Результаты помогут оценить применимость пакетной подписи в высоконагруженных PKI-системах.

**Это открытый PoC для портфолио и поиска партнёров. Код распространяется под лицензией MIT.**

## Возможности


- Реализация дерева Меркла в `src/merkle`
- Ядро пакетной подписи в `src/batch_signing*`
- Формат пакетной подписи: `подпись_корня + список_доказательств` (с метаданными для сериализации)
- Слой абстракции хеш-функций (поддерживает **Стрибог** и тестовые hash-backend'ы)
- Слой абстракции подписей для российских PQC:
  - [Шиповник](https://github.com/QAPP-tech/shipovnik_tc26)
  - [Гиперикум](https://github.com/QAPP-tech/hypericum_tc26)
  - [Крыжовник](https://github.com/ElenaKirshanova/pqc_LWR_signature)
- Последовательный бенчмарк (`bench_seq`) с параметрами paramset, verify-проходом и прогревочными итерациями
- Набор тестов с унифицированными именами `test_*` в CTest

## Оригинальные репозитории и форки

Ссылки на алгоритмы в этом README всегда ведут на оригинальные репозитории:

- Шиповник (оригинал): https://github.com/QAPP-tech/shipovnik_tc26
- Гиперикум (оригинал): https://github.com/QAPP-tech/hypericum_tc26
- Крыжовник (оригинал): https://github.com/ElenaKirshanova/pqc_LWR_signature

В проекте как сабмодули используются интеграционные/совместимые форки:

- https://github.com/cherninkiy/shipovnik-wrapper-tc26
- https://github.com/cherninkiy/hypericum-wrapper-tc26
- https://github.com/cherninkiy/kryzhovnik-wrapper-tc26

Зачем используются форки:

- Для добавления интеграционных API, необходимых проекту (detached и status-return обёртки)
- Для унификации контрактов адаптеров между тремя алгоритмами
- Для воспроизводимых зафиксированных ревизий в CI и локальной сборке

## Структура репозитория

```
batch-pqc/

├── third_party/            # git submodules
│   ├── shipovnik/          → https://github.com/cherninkiy/shipovnik-wrapper-tc26
│   ├── hypericum/          → https://github.com/cherninkiy/hypericum-wrapper-tc26
│   ├── kryzhovnik/         → https://github.com/cherninkiy/kryzhovnik-wrapper-tc26
├── src/
│   ├── signature/          # адаптеры для алгоритмов подписи
│   ├── batch_bench.h       # единый контракт адаптеров (bb_status, bb_algorithm)
│   ├── merkle/             # реализация дерева Меркла
│   ├── batch_signing.h     # API пакетной подписи
│   ├── batch_signing.c     # реализация пакетной подписи
│   ├── batch_adapters.h    # callback-интерфейс для интеграции с bb_algorithm API
│   ├── batch_adapters.c    # реализация callback-адаптеров
│   └── utils/              # таймеры, генераторы сообщений
├── bench/                  # исполняемые файлы бенчмарков
├── tests/                  # модульные тесты (CTest)
├── scripts/                # автоматизация сборки и бенчмарков
├── results/                # CSV и графики (генерируются автоматически)
└── docs/                   # архитектура и финальный отчёт
```

## Сборка и запуск (MVP)

### Требования

- Linux x86_64 (тестировалось на Ubuntu 22.04)
- CMake 3.14+, GCC/Clang с поддержкой C11
- OpenSSL development package (`libssl-dev`) для сборки Kryzhovnik

### Быстрый старт

```bash

# Клонирование с субмодулями
git clone --recursive https://github.com/cherninkiy/batch-pqc.git
cd batch-pqc

# Синхронизация URL сабмодулей после обновления .gitmodules
git submodule sync --recursive
git submodule update --init --recursive

# Сборка сторонних зависимостей и тестов
./scripts/third_party.sh build

# Запуск тестов
./scripts/third_party.sh tests

# Запуск бенчмарка (последовательная подпись)
./build/bench/bench_seq --algo hypericum --batch-size 16 --iters 100 --verify 1
```

### Выбор paramset

Для Hypericum и Kryzhovnik поддерживается выбор набора параметров на этапе сборки через CMake.

```bash
cmake -S . -B build \
  -DHYPERICUM_PARAMSET=m_128_20 \
  -DKRYZHOVNIK_PARAMSET=medium
cmake --build build --parallel
```

Поддерживаемые значения для Hypericum: `b_256_64`, `m_256_64`, `b_256_20`, `m_256_20`, `b_128_20`, `m_128_20`, `debug`.

`debug` — агрессивный исследовательский профиль, ориентированный на скорость для benchmark/diagnostics.

Поддерживаемые значения для Kryzhovnik: `small`, `medium`, `large`, `debug`.

`debug` предназначен для быстрой диагностики и отладки, а не для оценки стойкости.

В benchmark-скрипте можно задавать наборы отдельно:

```bash
./scripts/benchmark.sh \
  --algo kryzhovnik \
  --kryzhovnik-params small \
  --hypericum-params debug
```

Примечание: константы Kryzhovnik для `small/medium/large` синхронизированы со скриптом `security.sage`; перед production-использованием их нужно дополнительно верифицировать по официальной спецификации.

### Ожидаемые результаты (после MVP)

После запуска бенчмарков в директории `results/` появятся:

- `raw_data.csv` – замеры времени для каждого (алгоритм, размер пакета)
- `plots/speedup_vs_batch.png` – график ускорения для всех трёх алгоритмов

## Текущий статус (MVP)

- [x] Структура репозитория с субмодулями
- [x] Адаптеры для Шиповника / Гиперикума / Крыжовника с единым `bb_status`
- [x] Подключены detached/status-return API через wrapper-сабмодули
- [x] Последовательный бенчмарк `bench_seq` с warmup и исправленной метрикой размера подписей
- [x] Реализация пакетной подписи/верификации на основе дерева Меркла
- [x] Покрытие тестами: `test_adapters_smoke`, `test_adapters_batch`, `test_kryzhovnik*`, `test_merkle`, `test_batch_signing`, `test_streebog`
- [ ] Сравнение бенчмарков: последовательная vs реальная пакетная подпись
- [ ] Финальный отчёт (PDF)

**MVP разрабатывается в ветке `dev`. После завершения будет открыт Pull Request в `main` для ревью.**

## Участие и ревью

Мы приглашаем к техническому ревью:

- **Елену Киршанову** (автор Крыжовника)
- **Команду QAPP** (авторы Шиповника и Гиперикума)


Если вы являетесь одним из авторов, пожалуйста, ознакомьтесь с [Pull Request](https://github.com/cherninkiy/batch-pqc/pull/1) (когда он будет открыт) и оставьте свои комментарии. Для общих отзывов используйте GitHub Issues.

## Лицензия

Этот проект распространяется под **лицензией MIT** – подробности в файле [LICENSE](LICENSE).

Сторонние компоненты имеют свои лицензии:

- Реализации PQC – смотрите лицензии в каждом субмодуле

## Благодарности

- [QAPP-tech](https://github.com/QAPP-tech) – подписи Шиповник и Гиперикум (включая хеш Стрибог)
- [Elena Kirshanova](https://github.com/ElenaKirshanova) – оригинальная реализация Крыжовника: [pqc_LWR_signature](https://github.com/ElenaKirshanova/pqc_LWR_signature)
- В этом проекте используется форк совместимости для интеграции Крыжовника с Гиперикум и Шиповник

---

**Внимание:** Это открытый PoC для портфолио и поиска партнёров. Код распространяется под лицензией MIT. Не предназначено для production. Используйте на свой страх и риск.
