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

1. `TQueue* createQueue(int size)` -- tworzy nową kolejkę o maksymalnym rozmiarze `size`.

2. `void destroyQueue(TQueue *queue)` -- usuwa kolejkę `queue` i zwalnia pamięć przez nią zajmowaną.

3. `void subscribe(TQueue *queue, pthread_t thread)` -- dodaje subskrybenta `thread` do kolejki `queue`.

4. `void unsubscribe(TQueue *queue, pthread_t thread)` -- usuwa subskrybenta `thread` z kolejki `queue`.

5. `void addMsg(TQueue *queue, void *msg)` -- dodaje wiadomość `msg` do kolejki `queue`.

6. `void* getMsg(TQueue *queue, pthread_t thread)` -- pobiera wiadomość dla subskrybenta `thread` z kolejki `queue`.

7. `int getAvailable(TQueue *queue, pthread_t thread)` -- zwraca liczbę dostępnych wiadomości dla subskrybenta `thread` w kolejce `queue`.

8. `void removeMsg(TQueue *queue, void *msg)` -- zajmuje zamek, wywołuje funkcję unsafeRemoveMessage dla wiadomości `msg` z kolejki `queque`

9. `void unsafeRemoveMsg(TQueue *queue, void *msg)` -- funkcja pomocniczna, usuwa wiadomość `msg` z kolejki `queue`.

10. `void setSize(TQueue *queue, int size)` -- ustala nowy rozmiar kolejki `queue` na `size`.

# Algorytm / dodatkowy opis

Algorytm implementuje wzorzec publish-subscribe z użyciem współbieżnych struktur danych. Kolejka jest zabezpieczona mutexem `rw_lock`, a wątki synchronizują się za pomocą zmiennej warunkowej `cond`.

1. **Dodawanie wiadomości**:
   - Wątki dodające wiadomości (`writer_thread`) czekają, jeśli kolejka jest pełna.
   - Po dodaniu wiadomości, wszysyscy subskrybenci są informowane o nowej wiadomości.

2. **Pobieranie wiadomości**:
   - Wątki pobierające wiadomości (`subscriber_thread`) czekają, jeśli nie ma dostępnych wiadomości.
   - Po pobraniu wiadomości, subskrybent usuwa ją z kolejki.

3. **Subskrypcja i odsubskrypcja**:
   - Wątki mogą subskrybować i odsubskrybować kolejkę. Subskrypcja dodaje wątek do listy subskrybentów, a odsubskrypcja usuwa go z listy. Jeden wątek może dokonać tylko jednej subskrypcji na raz.

4. **Zmiana rozmiaru kolejki**:
   - Funkcja `setSize` umożliwia zmianę rozmiaru kolejki. Jeśli nowy rozmiar jest mniejszy, najstarsze wiadomości są usuwane.

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

-------------------------------------------------------------------------------













Informacje zawarte poniżej nie powinny już być zawarte w samym sprawozdaniu.


# Formatowanie dokumentu

Sprawozdanie powinno być zapisane w dokumencie tekstowym w formacie
[Markdown](https://www.markdownguide.org) lub
[reStructuredText](https://en.wikipedia.org/wiki/ReStructuredText). Należy
zastosować kodowanie znaków [UTF-8](https://en.wikipedia.org/wiki/UTF-8).

* **Markdown**: Bardzo dobre wprowadzenie do formatu Markdown można znaleźć na
  stronie [Basic Syntax](https://www.markdownguide.org/basic-syntax/).  Kwestie
  bardziej zaawansowane są omówione na oddzielnej stronie [Extended
  Syntax](https://www.markdownguide.org/extended-syntax/).  Proszę zwrócić uwagę
  na uwagi zawarte w punktach *Best Practices*, zwłaszcza chodzi o poprawne
  izolowanie od siebie kolejnych akapitów tekstowych/list.

* **reStructuredText**: Zwięzły opis formatu można znaleźć na stronie
[Wikipedii](https://en.wikipedia.org/wiki/ReStructuredText). Alternatywnie można
zapoznać się z dokumentacją pakietu [Docutils](https://www.docutils.org) do
przetwarzania dokumentów RST: [Quick
reStructuredText](https://www.docutils.org/docs/user/rst/quickref.html) lub
zerknąć na [reStructuredText
Guide](https://restructuredtext-guide.readthedocs.io).


## Tabelki

Formatowanie ewentualnych tabel można zrealizować z wykorzystaniem bardzo
wygodnego generatora [Tables Generator](https://www.tablesgenerator.com),
obsługującego bezpośrednio format
[Markdown](https://www.tablesgenerator.com/markdown_tables).  W przypadku
reStructuredText można wykorzystać po prostu [tabelki
tekstowe](https://www.tablesgenerator.com/text_tables).


## Fragmenty kodu

Fragmenty kodu załączamy aktywując kolorowanie składni:

```C
#include <stdio.h>

void main() {
    printf("Hello world!\n");
}
```


## Rysunki i diagramy

Najprostszym rozwiązaniem jest oczywiście użycie prostego [ASCII
art](https://en.wikipedia.org/wiki/ASCII_art), co ma tę niezaprzeczalną zaletę,
że rysunek można osadzić bezpośrednio w dokumencie.  Alternatywnie można odwołać
się do zewnętrznego pliku w formacie rastrowym (PNG, JPG, WebP).

![Przykładowy rysunek](rys.png){width=70%}


## Wzory matematyczne

Wzory matematyczne można zapisywać zgodnie z
[notacją](https://en.wikibooks.org/wiki/LaTeX/Mathematics)
[LaTeX-a](https://www.overleaf.com/learn/latex/Mathematical_expressions), np.:
$$\sum_{i=1}^{N}\frac{e^{-2\pi}}{\left(x_{i}-y_{i}\right)^{2}}$$

Dla początkujących może się okazać przydatny prosty edytor wzorów matematycznych
[on-line](https://latexeditor.lagrida.com).


# Walidacja formatowania

Błędne formatowanie sprawozdania może być przyczyną odrzucenia projektu, choć w
końcowym rozrachunku nie wpływa na ocenę.

Warto wspomnieć, że zarówno format Markdown jak i reStructuredText są
obsługiwane przez system [GitHub](https://github.com), którego lokalne wdrożenie
na bazie [GitLab](https://gitlab.com) jest dostępne pod adresem
<https://git.cs.put.poznan.pl>.

Optyczną weryfikację formatowania dokumentu można przeprowadzić z użyciem
przedstawionych poniżej przeglądarek lub edytorów formatów znacznikowych.

* Dla formatu Markdown:

  - Przeglądarka dokumentów [Okular](https://okular.kde.org)
  - [StackEdit](https://stackedit.io)
  - [Markdown Live Preview](https://markdownlivepreview.com)
  - [VisualStudio](https://code.visualstudio.com/Docs/languages/markdown)
  - [Typora](https://typora.io)

* Dla formatu reStructuredText:

  - [Documatt](https://snippets.documatt.com)
  - [Docutils](https://www.docutils.org)
  - [Sphinx](https://www.sphinx-doc.org)

## Pandoc

Na koniec warto wspomnieć o jeszcze jednym narzędziu:
[Pandoc](https://pandoc.org).  Ten konwerter potrafi bardzo skutecznie dokonać
translacji z jednego formatu znacznikowego do innego lub np. do formatu PDF.
Oto przykłady użycia tego narzędzia:

```sh
$ pandoc -s DOC.md -o DOC.rst
$ pandoc -s DOC.rst -o DOC.md
$ pandoc DOC.md -o DOC.pdf
```

Styl wynikowego dokumentu PDF można parametryzować definiując odpowiednie
zmienne, np.:

```sh
$ pandoc -N --pdf-engine xelatex \
>        --highlight-style tango \
>        -V papersize=a4 \
>        -V geometry:margin=2.5cm \
>        -V fontsize=11pt \
>        -V mainfont="Droid Serif" \
>        -V sansfont="Droid Sans" \
>        -V monofont="Droid Sans Mono" \
>        -V urlcolor=blue!70!black \
>        DOC.md -o DOC.pdf
```

Dla ułatwienia domyślne opcje konwersji można zapisać w pliku konfiguracyjnym w
formacie [YAML](https://pl.wikipedia.org/wiki/YAML), np.:

```yaml
pdf-engine: xelatex
highlight-style: tango
standalone: true
numbersections: true
variables:
  papersize: a4
  geometry:  margin=2.5cm
  fontsize:  11pt
  mainfont:  Droid Serif
  seriffont: Droid Serif
  sansfont:  Open Sans
  monofont:  Droid Sans Mono
  urlcolor:  blue!70!black
  sansfontoptions: Scale=MatchLowercase
```

Zakładając, że powyższy przykład zostanie zapisany w pliku `MYDEFS.yaml` i
umieszczony w katalogu `~/.pandoc/defaults/`, możliwe będzie przeprowadzenie
konwersji poniższym wywołaniem:

```sh
$ pandoc -d MYDEFS DOC.md -o DOC.pdf
```