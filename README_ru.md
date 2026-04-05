
# batch-pqc

**PoC: Пакетная подпись для российских пост-квантовых криптоалгоритмов**

[![Лицензия: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![PRs Приветствуются](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](CONTRIBUTING.md)
[![Статус CI](https://github.com/cherninkiy/batch-pqc/actions/workflows/ci.yml/badge.svg)](https://github.com/cherninkiy/batch-pqc/actions/workflows/ci.yml)

**[English version](README.md)**

## Обзор


Этот проект демонстрирует **пакетную подпись** (на основе дерева Меркла) для трёх российских пост-квантовых схем подписи:

- **Шиповник** (на основе протокола Штерна)
- **Гиперикум** (stateless, на основе SPHINCS+, использует хеш-функцию **Стрибог** из своей реализации)
- **Крыжовник** (на основе решёток, аналог Dilithium)

Цель — измерить ускорение при подписи пакета сообщений с помощью одной подписи корня дерева + доказательств по сравнению с последовательной подписью. Результаты помогут оценить применимость пакетной подписи в высоконагруженных PKI-системах.

**Это открытый PoC для портфолио и поиска партнёров. Код распространяется под лицензией MIT.**

## Возможности


- Реализация дерева Меркла (обёртка над [IAIK/merkle-tree](https://github.com/IAIK/merkle-tree))
- Слой абстракции хеш-функций (поддерживает **Стрибог** через реализацию Hypericum, SHA-256 для тестов)
- Слой абстракции подписей:
  - Российские PQC: [Шиповник](https://github.com/QAPP-tech/shipovnik_tc26), [Гиперикум](https://github.com/QAPP-tech/hypericum_tc26), [Крыжовник](https://github.com/ElenaKirshanova/pqc_LWR_signature)
  - (планируется) ML-DSA (FIPS 204) через liboqs
- API для пакетной подписи и верификации
- Микробенчмарки: последовательная vs пакетная подпись, накладные расходы на верификацию
- Экспорт результатов в CSV и графики

## Структура репозитория

```
batch-pqc/

├── third_party/            # git submodules
│   ├── shipovnik/          → https://github.com/QAPP-tech/shipovnik_tc26
│   ├── hypericum/          → https://github.com/QAPP-tech/hypericum_tc26
│   ├── kryzhovnik/         → https://github.com/ElenaKirshanova/pqc_LWR_signature
│   └── iaik_merkle_tree/   → https://github.com/IAIK/merkle-tree
├── src/
│   ├── hash/               # провайдер хешей (Стрибог из hypericum, SHA-256 как запасной)
│   ├── signature/          # адаптеры для алгоритмов подписи
│   ├── merkle/             # обёртка Merkle над IAIK
│   ├── batch/              # логика пакетной подписи
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
- OpenSSL (для SHA-256, опционально)

### Быстрый старт

```bash

# Клонирование с субмодулями
git clone --recursive https://github.com/cherninkiy/batch-pqc.git
cd batch-pqc

# Сборка
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Запуск тестов
ctest --output-on-failure

# Запуск бенчмарков (последовательная vs пакетная)
./bench/bench_batch --algo hypericum --batch-size 16 --iterations 100
```

### Ожидаемые результаты (после MVP)

После запуска бенчмарков в директории `results/` появятся:

- `raw_data.csv` – замеры времени для каждого (алгоритм, размер пакета)
- `plots/speedup_vs_batch.png` – график ускорения для всех трёх алгоритмов

## Текущий статус (MVP)

- [ ] Структура репозитория с субмодулями
- [ ] Абстракция хешей + интеграция Стрибога (из hypericum)
- [ ] Обёртка Merkle дерева (использует IAIK)
- [ ] Пакетная подпись/верификация с нулевой подписью (тестовый режим)
- [ ] Адаптеры для реальных PQC-алгоритмов (в процессе: сначала Гиперикум)
- [ ] Полный набор бенчмарков с результатами
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

- `iaik_merkle_tree` – public domain / BSD-like
- Реализации PQC – смотрите лицензии в каждом субмодуле

## Благодарности

- [IAIK/merkle-tree](https://github.com/IAIK/merkle-tree) – реализация дерева Меркла (public domain / BSD‑like)
- [QAPP-tech](https://github.com/QAPP-tech) – подписи Шиповник и Гиперикум (включая хеш Стрибог)
- [Elena Kirshanova](https://github.com/ElenaKirshanova) – подпись Крыжовник (на решётках)

---

**Внимание:** Это открытый PoC для портфолио и поиска партнёров. Код распространяется под лицензией MIT. Не предназначено для production. Используйте на свой страх и риск.
