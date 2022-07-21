# HOME TASK

## Сборка и запуск

```(bash)
./build.sh
./build/bin/client_server
```

## Принцип работы в общих словах

Клетка массива задается двумя значениями: индентификатор клетки и хранимое значение.
При удалении значения в запросе указывается только индентиикатор клетки.
При вставке значения указывается индентификатор клетки после которой планируем встать.
Индентификатор начинается с 1, поэтому при указании 0, вставка произойдет в начало массива.

Индентификатор идет по порядку, но по хорошему надо реализовать пул свобоодных диапазонов индентификаторов и уже из него выдвать значения.

Состояние массива хранится в виде сохраненного стейта + история изменений + номер итерации.
При каждом изменении номер итерации увеличивается на 1.

Каждый запрос это сама команда и последний известный номер итерации на сервере.

Каждый ответ, это история изменений после номера итерации в запросе + номер итерации.

На сервере так же хранится список клиентов с последним подтвержденным номером итерации полученным клиентом.
Инвалидация подтвержденных номеров по времени или по размеру истории не реализована.

Сервер при получении запроса:
1. сдвигает подтвержденным номером итерации полученным клиентом,
2. добавляет к ответу историю о которой не знает клиент и текущий номер итерации
3. отправляет ответ
4. применяет хвост истории(изменения которые известны всем клиентам) к стейту массива

Первенство запросов определяется тем кого первее начал обрабатывать сервер, а не по времени создания.

Клиент при получении запроса:
1. применяет полученную часть истории к своему стейту массива
2. cохраняет номер итерации

Таким образом по сети передается только дифф изменений.

## Стейт массива сервера

<Переписывается из-за переписанного решения>

## Стейт массива клиента

Стейт массива для клиента состоит из следующих контейнеров:
1. Сбалансирвованное дерево поиска, где вместо ключа индекс элемента (для примера используеся декартово дерево, но лучше использовать другое дерево) 
2. Хеш-таблица {индентификатор клетки, итератор на клетку в дереве}

Дерев поиска позволяет относительно быстро получать доступ к рандомному элементe, а хеш-таблица получать элемент по его ключу.

Генерация запросов:
O(log<количествоо элементов в масссиве>)

Применение запросов:
O(log<количествоо элементов в масссиве>)

Применение стейта:
O(log<количествоо элементов в масссиве> * <количествоо элементов в масссиве>)
В идеале можно реализовать через стек построение декартова дерева в O(<количествоо элементов в масссиве>)


## Результаты

Из минус, большое потребление RAM; В прогоне потребляет примерно 27GB на сервер и всех клиентов.
При теоретическом минимуме 4 * 10^7 * 21 = 840MB
Выходит из того, что ноды используемых контейнеров занимают гораздо больше места чем само значение

Без учета первой выгрузки стейта
SBPS - SentBytesPerSecond
RBPS - ReceivedBytesPerSecond
```
Id# 1 SentBytes# 28.36kB ReceivedBytes# 153.45kB
 SBPS# 284.70B/s RBPS# 1.54kB/s
 Iterations# 484 IterationsPerSecond# 4.86 WorkTime# 99.61s
Id# 2 SentBytes# 27.77kB ReceivedBytes# 153.14kB
 SBPS# 280.14B/s RBPS# 1.54kB/s
 Iterations# 484 IterationsPerSecond# 4.88 WorkTime# 99.12s
Id# 3 SentBytes# 27.99kB ReceivedBytes# 152.94kB
 SBPS# 285.23B/s RBPS# 1.56kB/s
 Iterations# 484 IterationsPerSecond# 4.93 WorkTime# 98.14s
Id# 4 SentBytes# 28.20kB ReceivedBytes# 153.07kB
 SBPS# 283.55B/s RBPS# 1.54kB/s
 Iterations# 484 IterationsPerSecond# 4.87 WorkTime# 99.45s
Id# 5 SentBytes# 27.75kB ReceivedBytes# 153.21kB
 SBPS# 279.53B/s RBPS# 1.54kB/s
 Iterations# 484 IterationsPerSecond# 4.88 WorkTime# 99.28s
Id# 6 SentBytes# 27.74kB ReceivedBytes# 152.95kB
 SBPS# 282.15B/s RBPS# 1.56kB/s
 Iterations# 484 IterationsPerSecond# 4.92 WorkTime# 98.30s
Id# 7 SentBytes# 28.03kB ReceivedBytes# 153.11kB
 SBPS# 283.74B/s RBPS# 1.55kB/s
 Iterations# 484 IterationsPerSecond# 4.90 WorkTime# 98.79s
Id# 8 SentBytes# 28.15kB ReceivedBytes# 153.26kB
 SBPS# 284.51B/s RBPS# 1.55kB/s
 Iterations# 484 IterationsPerSecond# 4.89 WorkTime# 98.95s
Id# 9 SentBytes# 27.87kB ReceivedBytes# 153.10kB
 SBPS# 282.59B/s RBPS# 1.55kB/s
 Iterations# 484 IterationsPerSecond# 4.91 WorkTime# 98.63s
Id# 10 SentBytes# 28.04kB ReceivedBytes# 153.31kB
 SBPS# 284.77B/s RBPS# 1.56kB/s
 Iterations# 484 IterationsPerSecond# 4.92 WorkTime# 98.46s
Id# 11 SentBytes# 28.15kB ReceivedBytes# 153.26kB
 SBPS# 287.37B/s RBPS# 1.56kB/s
 Iterations# 484 IterationsPerSecond# 4.94 WorkTime# 97.96s
Id# 12 SentBytes# 28.02kB ReceivedBytes# 153.34kB
 SBPS# 286.53B/s RBPS# 1.57kB/s
 Iterations# 484 IterationsPerSecond# 4.95 WorkTime# 97.80s
Id# 13 SentBytes# 28.30kB ReceivedBytes# 153.32kB
 SBPS# 292.38B/s RBPS# 1.58kB/s
 Iterations# 484 IterationsPerSecond# 5.00 WorkTime# 96.80s
Id# 14 SentBytes# 28.08kB ReceivedBytes# 153.12kB
 SBPS# 287.58B/s RBPS# 1.57kB/s
 Iterations# 484 IterationsPerSecond# 4.96 WorkTime# 97.64s
Id# 15 SentBytes# 28.21kB ReceivedBytes# 153.30kB
 SBPS# 289.39B/s RBPS# 1.57kB/s
 Iterations# 484 IterationsPerSecond# 4.97 WorkTime# 97.48s
Id# 16 SentBytes# 28.20kB ReceivedBytes# 153.16kB
 SBPS# 289.80B/s RBPS# 1.57kB/s
 Iterations# 484 IterationsPerSecond# 4.97 WorkTime# 97.31s
Id# 17 SentBytes# 28.20kB ReceivedBytes# 153.15kB
 SBPS# 290.33B/s RBPS# 1.58kB/s
 Iterations# 484 IterationsPerSecond# 4.98 WorkTime# 97.13s
Id# 18 SentBytes# 27.98kB ReceivedBytes# 153.15kB
 SBPS# 288.50B/s RBPS# 1.58kB/s
 Iterations# 484 IterationsPerSecond# 4.99 WorkTime# 96.97s
Id# 19 SentBytes# 28.25kB ReceivedBytes# 153.46kB
 SBPS# 292.02B/s RBPS# 1.59kB/s
 Iterations# 484 IterationsPerSecond# 5.00 WorkTime# 96.73s
Id# 20 SentBytes# 28.23kB ReceivedBytes# 153.26kB
 SBPS# 292.46B/s RBPS# 1.59kB/s
 Iterations# 483 IterationsPerSecond# 5.00 WorkTime# 96.53s
```

## Тесты

Как-то без тестов получилось

P.S. Проверка проводилась путем проверки стейтов клиентов и сервера.
