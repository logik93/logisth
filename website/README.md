# wavelogik.pl

Strona promuje syntezator logisth. Zawiera polski opis instrumentu, angielski podręcznik użytkownika i dokumentację techniczną w HTML oraz PDF. Czcionki, grafiki i instrukcje są częścią paczki. Przyciski pobierania instrumentu prowadzą do wydania na GitHubie.

## Wgranie na własny serwer

Pobierz [paczkę strony](wavelogik.pl.zip) i rozpakuj jej zawartość do katalogu witryny przypisanego do domeny `wavelogik.pl`. Plik `index.html` powinien znajdować się na głównym poziomie tego katalogu, obok `assets/`, `docs/`, `technical/`, `site.css` i `site.js`.

Witryna działa jako statyczne pliki na Apache lub Nginx, również na serwerze z PHP. Jeżeli w katalogu jest już strona z `index.php`, wybierz osobny katalog dla tej witryny lub dostosuj kolejność plików startowych w ustawieniach hosta. Domena i certyfikat HTTPS wymagają konfiguracji na własnym hostingu.

Wejdź na stronę główną, a następnie otwórz `docs/` i `technical/`. Z obu podręczników można pobrać lokalny PDF. Względne ścieżki pozwalają również sprawdzić witrynę w podkatalogu.

## Aktualizowanie dokumentacji

Źródła angielskich podręczników oraz ich wydania PDF znajdują się w katalogu `docs/` repozytorium. Po aktualizacji PDF uruchom z katalogu głównego repozytorium poniższe polecenia. Do generowania dokumentacji potrzebne są Python 3 z Pillow i Pandoc.

```sh
python website/build_docs.py
python website/package.py
```

Skrypt pakujący zapisuje wszystkie pliki z `website/public/` w archiwum `website/wavelogik.pl.zip`. Przed nowym wydaniem zaktualizuj też numer wersji i adresy pobierania w `website/public/index.html`.
