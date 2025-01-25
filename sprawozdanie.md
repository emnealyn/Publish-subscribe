---
title:    Publish-subscribe
subtitle: Programowanie systemowe i współbieżne
author:   Emilia Bonk 160108 <emilia.bonk@student.put.poznan.pl>
date:     v1.0, 2024-01-20
lang:     pl-PL
---

# Struktury danych

W tym punkcie przedstawiam struktury danych zaprojektowane i użyte w implementacji. Opis uwzględnia prezentację istotnych zmiennych i możliwych wartości składowanych w nich.

1. Struktura `Message` reprezentuje wiadomość w kolejce:

   ```c
   struct Message {
       pthread_mutex_t lock;
       void *data;
       int remaining_read_count;
       struct Message *next;
   };
   ```

   - `lock`: mutex do synchronizacji dostępu do wiadomości.
   - `data`: wskaźnik na dane wiadomości.
   - `remaining_read_count`: liczba subskrybentów, którzy jeszcze nie przeczytali wiadomości.
   - `next`: wskaźnik na następną wiadomość w kolejce.

2. Struktura `Subscriber` reprezentuje subskrybenta kolejki:

   ```c
   struct Subscriber {
       pthread_t thread;
       Message *head;
       struct Subscriber *next;
   };
   ```

   - `thread`: identyfikator wątku subskrybenta.
   - `head`: wskaźnik na pierwszą wiadomość do przeczytania przez subskrybenta.
   - `next`: wskaźnik na następnego subskrybenta w liście.

3. Struktura `TQueue` reprezentuje kolejkę:

   ```c
   struct TQueue {
       int max_size;
       int current_size;
       Message *head;
       Message *tail;
       pthread_mutex_t rw_lock;
       pthread_cond_t cond;
       int subscriber_count;
       Subscriber *subscribers;
   };
   ```

   - `max_size`: maksymalny rozmiar kolejki.
   - `current_size`: aktualny rozmiar kolejki.
   - `head`: wskaźnik na pierwszą wiadomość w kolejce.
   - `tail`: wskaźnik na ostatnią wiadomość w kolejce.
   - `rw_lock`: mutex do synchronizacji dostępu do kolejki.
   - `cond`: zmienna warunkowa do synchronizacji wątków.
   - `subscriber_count`: liczba subskrybentów.
   - `subscribers`: wskaźnik na listę subskrybentów.

# Funkcje

Poniżej znajdują się opisy funkcji zawartych w implementacji:

1. `void removeMsg(TQueue *queue, void *msg)` -- zajmuje zamek, wywołuje funkcję unsafeRemoveMessage dla wiadomości `msg` z kolejki `queque`

2. `void unsafeRemoveMsg(TQueue *queue, void *msg)` -- funkcja pomocniczna, usuwa wiadomość `msg` z kolejki `queue`.

# Algorytm / dodatkowy opis

Algorytm implementuje wzorzec publish-subscribe z użyciem współbieżnych struktur danych. Kolejka jest zabezpieczona mutexem `rw_lock`, a wątki synchronizują się za pomocą zmiennej warunkowej `cond`.

1. **Dodawanie wiadomości**:
   - Wątki dodające wiadomości (`writer_thread`) czekają, jeśli kolejka jest pełna.
   - Po dodaniu wiadomości, wszyscy subskrybenci są informowani o nowej wiadomości.

2. **Pobieranie wiadomości**:
   - Wątki pobierające wiadomości (`subscriber_thread`) czekają, jeśli nie ma dostępnych wiadomości.
   - Po pobraniu wiadomości, subskrybent usuwa ją z kolejki.

3. **Subskrypcja i odsubskrypcja**:
   - Wątki mogą subskrybować i odsubskrybować kolejkę. Subskrypcja dodaje wątek do listy subskrybentów, a odsubskrypcja usuwa go z listy. Jeden wątek może dokonać tylko jednej subskrypcji na raz.

4. **Zmiana rozmiaru kolejki**:
   - Funkcja `setSize` umożliwia zmianę rozmiaru kolejki. Jeśli nowy rozmiar jest mniejszy, najstarsze wiadomości są usuwane.

5. **Obsługa przypadków brzegowych**:
   - Jeśli kolejka jest pusta, wątki pobierające wiadomości czekają na zmiennej warunkowej `cond`.
   - Jeśli kolejka jest pełna, wątki dodające wiadomości czekają na zmiennej warunkowej `cond`.

6. **Odporność na problemy współbieżności**:
   - Algorytm unika zakleszczeń (deadlock) poprzez stosowanie mutexów i zmiennych warunkowych w odpowiednich miejscach.
   - Algorytm unika aktywnego oczekiwania (busy waiting) poprzez użycie zmiennych warunkowych.
   - Algorytm unika zagłodzenia (starvation) poprzez odpowiednie zarządzanie kolejką i subskrybentami.

# Przykład użycia

Przykładowe uruchomienie programu:

```sh
$ ./queue_program
```

Program tworzy kolejkę o rozmiarze 10 i uruchamia 30 wątków dodających wiadomości oraz 30 wątków pobierających wiadomości. Wątki synchronizują się za pomocą mutexów i zmiennych warunkowych, aby zapewnić poprawne działanie kolejki.

```c
int main() {
    TQueue *q = createQueue(10);

    #define NUM 30
    pthread_t wlist[NUM];
    pthread_t rlist[NUM];
    for (int i = 0; i < NUM; i++) {
        pthread_create(&wlist[i], NULL, subscriber_thread, (void*)q);
        pthread_create(&rlist[i], NULL, writer_thread, (void*)q);
    }

    for (int i = 0; i < NUM; i++) {
        pthread_join(wlist[i], NULL);
        pthread_join(rlist[i], NULL);
    }
    destroyQueue(q);
    return 0;
}
```