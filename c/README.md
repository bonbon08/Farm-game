# Farmi (C / raylib)

C-Neuimplementierung des Farmspiels mit [raylib](https://www.raylib.com), einem
Hauptmenue zur Auswahl von 1-4 Spielern, lokalem Splitscreen-Coop und ein paar
zusaetzlichen Spielsystemen gegenueber der urspruenglichen Python-Version.

## Bauen & Starten

```
cd c
make run
```

Benoetigt `raylib` (pkg-config `raylib`) und einen C11-Compiler.

## Hauptmenue

Beim Start waehlt man mit `<`/`>` (Pfeiltasten, A/D, Maus-Klick oder Gamepad 1
D-Pad) die Spieleranzahl (1-4) und startet mit Enter/Leertaste/Maus-Klick/
Gamepad-A. Das Menue zeigt live an, ob die benoetigten Controller erkannt
wurden. Waehrend des Spiels bringt ESC (sofern der Shop geschlossen ist)
zurueck ins Menue.

## Steuerung

| Spieler | Bewegen           | Interagieren        | Shop oeffnen   |
|---------|-------------------|---------------------|----------------|
| 1       | WASD              | Leertaste **oder** Linksklick auf ein Feld | B |
| 2       | Gamepad 1 Stick   | Gamepad 1 Taste A   | Gamepad 1 Start|
| 3       | Gamepad 2 Stick   | Gamepad 2 Taste A   | Gamepad 2 Start|
| 4       | Gamepad 3 Stick   | Gamepad 3 Taste A   | Gamepad 3 Start|

Spieler 1 zielt mit der Maus: der Cursor im eigenen Viewport markiert das Feld,
das angeklickt wird - unabhaengig davon, wo der Charakter gerade steht oder
hinschaut. Alle anderen Spieler interagieren mit dem Feld, dem ihr Charakter
gerade zugewandt ist.

Im Shop: Pfeile/WS/D-Pad zum Auswaehlen, Leertaste/Enter/Gamepad-A zum Kaufen -
oder einfach mit der Maus eine Zeile anklicken. Ein Klick ausserhalb des
Shop-Fensters schliesst ihn, genau wie ESC/Start.

## Splitscreen-Layout

- 1 Spieler: Vollbild
- 2 Spieler: geteilt links/rechts
- 3-4 Spieler: Viertel-Raster (bei 3 Spielern bleibt ein Feld leer)

Jeder Viewport hat eine eigene, am Spieler haengende Kamera, zeigt aber die
gemeinsame Welt inklusive aller anderen Spieler.

## Speicherstand

Der Spielstand (Geld, Upgrades, Tag, Feld, Quest-Fortschritt, Hof-Level und
Erfolge) wird automatisch alle 30 Sekunden sowie beim Zurueckkehren ins Menue
(ESC) und beim Beenden in `c/savegame.dat` gesichert. Existiert eine Speicher-
datei, zeigt das Hauptmenue zusaetzlich zu "Neues Spiel" einen "Fortsetzen"-
Button. Hof-Level und Erfolge sind account-weit: ein neues Spiel setzt nur den
Hof (Feld, Geld, Tag, Quest) zurueck, nicht den langfristigen Fortschritt.

## Sound & Musik

Alle Sounds werden beim Start prozedural erzeugt (einfache Sinus-/Rechteck-
Toene mit Lautstaerke-Huellkurve) - es werden keine externen Audiodateien
benoetigt. Es gibt kurze Effekte fuers Pflanzen, Ernten, Unkraut entfernen,
Upgrades kaufen, Quests/Erfolge abschliessen und UI-Interaktion, dazu einen
weichen Ambient-Pad-Loop im Hintergrund. `M` schaltet den Ton komplett stumm
(wirkt sowohl im Menue als auch im Spiel). Im Hauptmenue gibt es zusaetzlich
einen Lautstaerkeregler (unten rechts, per Maus ziehen) fuer eine stufenlose
Gesamtlautstaerke statt nur An/Aus.

## Warum farmen? (Motivation / Progression)

- **Hof-Level & Titel**: Jedes verdiente Gold (aus Ernten und Quests) zaehlt
  lebenslang mit und schaltet Titel frei - 21 Stufen von "Hobbygaertner" bis
  "Gemuese-Singularitaet" bei 100 Millionen lebenslang verdientem Gold (die
  Leiter wurde deutlich verlaengert, weil das alte 6-Stufen-System bei nur
  3000 Gold endete und durch die Gemuesesorten-Preise im Millionenbereich
  sofort trivial erreicht war). Level und Fortschrittsbalken sind sowohl im
  Menue als auch im HUD waehrend des Spiels sichtbar.
- **Erfolge**: 10 freischaltbare Erfolge (erste Ernte, 50 Ernten, 1000 Gold
  lebenslang verdient, Tag 5 erreicht, ein Upgrade maximiert, 10 Quests
  abgeschlossen, zu viert gespielt, erste Drohne, voll automatisiert inklusive
  aller Drohnen und Maeher, alle Gemuesesorten freigeschaltet) - einsehbar
  ueber den "Erfolge"-Button im Hauptmenue, mit Toast- und
  Sound-Benachrichtigung beim Freischalten.
- **Automatisierung**: Mehrere Shop-Upgrades bauen die Farm zum Idle-Betrieb
  aus, jedes mit einer klaren, nicht ueberlappenden Aufgabe:
  - **Auto-Farmarbeit** (bis zu 8 Stufen) arbeitet regelmaessig die Felder
    direkt um jeden Spieler herum ab, als wachsende Raute: Stufe 1 die 4
    Felder orthogonal neben dem Farmer, Stufe 2 zusaetzlich den naechsten Ring
    aus 8 Feldern (zusammen die "zwei vollen Ringe", 12 Felder), jede weitere
    Stufe einen zusaetzlichen Ring. Leere Felder in Reichweite werden
    bepflanzt, reife Pflanzen automatisch geerntet - Unkraut fasst dieses
    System nicht an, das ist allein Aufgabe der Maeher (siehe unten).
  - **Farm-Drohnen** (bis zu 30 kaufbar, steigender Preis): fliegen
    eigenstaendig los und arbeiten nach Prioritaet - zuerst die naechste reife
    Pflanze ernten, sonst eine ueberreife Pflanze retten (gleiche Auszahlung,
    aber gut 2,5x langsamer zu verarbeiten als eine frisch reife - "sammeln,
    nur langsamer"), sonst ein freies Feld bepflanzen (sichtbar als kleine
    schwebende Einheiten mit rotierenden Rotoren, die Augenfarbe zeigt die
    aktuelle Aufgabe: gold=ernten, orange=ueberreif retten, gruen=pflanzen).
    Mehrere Drohnen sprechen sich ab und meiden bereits von einer anderen
    Drohne anvisierte Felder, statt alle auf dasselbe Ziel zuzufliegen.
    Drohnen ruehren Unkraut nicht an, das bleibt Aufgabe der Maeher.
  - **Drohnen-Tempo** (bis Stufe 60): erhoeht Flug- und Arbeitsgeschwindigkeit
    aller Drohnen gleichzeitig um +8% pro Stufe.
  - **Rasenmaeher** (bis zu 30 kaufbar, steigender Preis): die einzigen
    Einheiten, die Unkraut **und** ueberreife, verwelkte Pflanzen entfernen -
    alles "schlechte Zeug" auf dem Feld. Jeder Maeher faehrt eine eigene
    Zeilen-"Bahn" ueber die ganze Karte ab - von oben links nach rechts, dann
    eine Zeile tiefer, immer weiter ("Rasenmaeher-Muster"). Mehrere Maeher
    bekommen automatisch unterschiedliche Zeilen zugewiesen, damit sie sich
    nicht gegenseitig die Arbeit wegnehmen oder dieselbe Stelle doppelt
    abfahren. Unkraut kostet wie gewohnt etwas Geld beim Entfernen, ueberreife
    Pflanzen werden einfach kostenlos geraeumt (wie eine manuelle Ernte zu
    spaet). Mäher sind mit Abstand das schnellste Automatisierungssystem im
    Spiel: Basisgeschwindigkeit und Tempo-Zuwachs pro Stufe liegen beide
    deutlich ueber denen von Spielern und Drohnen, damit ein voll
    aufgeruesteter Maeher am Ende sichtbar ueber das Feld rast. (Behoben: bei
    sehr hohem Tempo blieben Maeher vorher sichtbar "stehen" - sie bewegten
    sich pro Frame weiter als die Restdistanz zum Ziel und schossen staendig
    daran vorbei, ohne je anzukommen. Bewegung wird jetzt auf die Restdistanz
    begrenzt, das Problem tritt bei keiner Geschwindigkeit mehr auf - betraf
    aus demselben Grund auch Drohnen und wurde dort ebenfalls behoben.)
  - **Maeher-Tempo** (bis Stufe 60): erhoeht die Fahrgeschwindigkeit aller
    Maeher um +15% pro Stufe (steilere Kurve als alle anderen Tempo-Upgrades).
  - **Maeher-Reichweite** (bis Stufe 16): vergroessert den Radius, in dem ein
    Maeher Unkraut/ueberreife Pflanzen um seine aktuelle Position herum
    gleichzeitig erfasst (Stufe 0 = nur das eine Feld, auf dem er gerade
    steht; Stufe 16 = ein 33x33-Bereich).
  - **Unkraut-Resistenz** (bis Stufe 60): senkt die Grundwahrscheinlichkeit,
    dass eine Pflanze ueberhaupt zu Unkraut wird, um 3% pro Stufe (Minimum:
    10% der urspruenglichen Chance bleibt immer bestehen).

  Das kostet weiterhin die ueblichen Saatgut-/Unkraut-Entfernungspreise, spart
  aber Klicks und ist ein Kern der Lang-Zeit-Motivation. Lauftempo, Wachstum,
  Verkaufspreis, Drohnen-Tempo, Maeher-Tempo, Unkraut-Resistenz, Scheune und
  Vogelscheuche gehen bis Stufe 60; Auto-Farmarbeit bleibt bei Stufe 18 und
  Maeher-/Drohnen-Reichweite bei Stufe 16 - diese radius-basierten Upgrades
  bewusst *nicht* mit angehoben, weil ihre Flaeche quadratisch mit der Stufe
  waechst und bei Stufe 60 laengst absurd gross waere. Die Kostenkurve bleibt
  trotzdem gemaessigt, damit das Endgame erreichbar bleibt statt astronomisch
  zu werden.

- **Gemuesesorten (Prestige, nicht mehr destruktiv)**: Nach Weizen (Start)
  lassen sich Kartoffeln, Karotten, Zucchini und danach endlos weitere
  "Mysterioese Gemuese"-Sorten freischalten - ein eigener Shop-Eintrag im
  Dorfladen. Die erste Freischaltung (Kartoffeln) kostet 1.000.000 Gold, jede
  weitere Sorte kostet das 2,5-fache der vorherigen, verkauft sich aber
  dauerhaft fuer das 3-fache der vorherigen (Kartoffeln = 3x, Karotten = 9x,
  Zucchini = 27x, und so weiter) - die Kostenkurve alleine macht zweistellige
  Stufen zu einem Ziel fuer viele, viele Spielstunden, nicht zu einer harten
  Wand. **Wichtig:** Freischalten setzt nichts mehr zurueck. Wachstum und
  Verkaufspreis werden pro Sorte getrennt verfolgt (jede Sorte hat ihre
  eigenen zwei Upgrade-Stufen, die einfach dort stehen bleiben, wo man sie
  zuletzt hingebracht hat) und geteilte Infrastruktur - Lauftempo, Auto-
  Farmarbeit, alle Drohnen, Maeher und deren Tempo/Reichweite - bleibt beim
  Freischalten komplett unangetastet. Jedes bereits gepflanzte Feld behaelt
  ausserdem dauerhaft die Sorte, mit der es gepflanzt wurde: schaltet man
  waehrend eine Weizen-Pflanze noch waechst auf Kartoffeln um, waechst und
  verkauft sich diese eine Pflanze trotzdem weiter als Weizen - nur neu
  gepflanzte Felder folgen der gerade aktiven Sorte. Sobald mehr als eine
  Sorte freigeschaltet ist, erscheint unten im Spiel eine kleine Leiste mit
  einem gezeichneten Symbol pro Sorte (die Knoepfe sind die Pflanzen, nicht
  nur Text) zum kostenlosen Umschalten (Maus-Klick oder Zifferntasten 1-4).
  Die ersten vier Sorten haben je ein eigenes, von Hand gezeichnetes Symbol
  (Weizen = goldene Aehren, Kartoffeln = braune Knollen, Karotten = oranges
  Wurzelgemuese mit gruenem Gruen, Zucchini = dunkelgruene Kapsel); jede
  Mysterioese Sorte danach bekommt eine eigene, von der Stufe abhaengige
  Farbe, damit sie optisch auseinanderzuhalten bleiben, auch ohne fuer jede
  einzeln neue Grafik von Hand zu zeichnen.
- **Scheune**: ein Upgrade (bis Stufe 60), das unabhaengig vom aktiven Ernten
  laufend etwas Gold einbringt, skaliert mit dem Hof-Level - ein Grund, auch
  in Zeiten ohne aktives Spielen etwas voranzukommen.
- **Feld-Erweiterung**: ein Upgrade (bis Stufe 10), das die nutzbare
  Feldflaeche schrittweise von 50x30 auf bis zu 90x50 Felder vergroessert.
  Spieler koennen sich frei auf der gesamten (groesseren) Karte bewegen,
  auch ins noch gesperrte Gebiet - dort laesst sich aber nichts pflanzen,
  und es wird sichtbar als geschlossener, dunkler Boden dargestellt statt
  als normales Ackerland, bis die naechste Stufe es freigibt.
- **Drohnen-Schwarmradius**: ein Upgrade (bis Stufe 16) fuer Drohnen, analog
  zur Maeher-Reichweite - sobald die maximale Drohnenzahl (30) erreicht ist,
  ist das der naechste Ort, um weiter zu investieren: jede Drohne erledigt
  bei jedem Stopp zusaetzlich alle passenden Felder in ihrem Radius, nicht
  nur das eine Zielfeld.
- **Voegel, Vogelscheuchen & Hunde**: Voegel sind ein nicht kaufbarer
  Schaedling - sie tauchen von selbst auf, zielen auf das naechste reife (ab
  Tag 10 auch keimende) Feld und richten beim Landen **grossflaechigen**
  Schaden an: alles Reife (bzw. Keimende) in einem Radius um die Landestelle
  wird vernichtet, nicht nur das eine Feld. Dieser Radius waechst mit den
  Spieltagen von 3x3 (Tag 1) bis auf 9x9 (ab Tag 19) - ein einzelner Vogel
  kann spaet im Spiel also ein ganzes Feldstueck auf einen Schlag zerstoeren.
  Wie oft sie ueberhaupt auftauchen, steigt ebenfalls deutlich schneller mit
  der Zeit als zuvor (Pruefintervall faellt von 6,4s an Tag 1 auf 1s ab etwa
  Tag 28). **Vogelscheuchen** (Upgrade, bis Stufe 60) senken rein passiv, wie
  oft ueberhaupt ein Vogel auftaucht, und stehen sichtbar als Holzpfosten mit
  Sack-Kopf am oberen Feldrand (einer pro Stufe, bis zu 24 gleichzeitig
  sichtbar). **Hunde** (kaufbar, bis zu 20, ebenfalls sichtbar auf dem Feld)
  sind die aktive Verteidigung: sie patrouillieren und jagen bereits
  aufgetauchte Voegel, bevor die ihr Ziel erreichen.
- **Verschleiss & Reparatur**: Drohnen und Maeher nutzen sich mit jeder
  erledigten Aktion ein kleines Stueck ab und koennen dadurch kaputt gehen -
  sie bleiben dann einfach stehen (grau, mit einem roten X statt Rotor/
  Klinge), bis sie repariert werden. Der Verschleiss pro Aktion ist bewusst
  gering (ein Geraet haelt im Schnitt 150-250 Aktionen durch, nicht nur ~40
  wie in einer frueheren Version) - ein kaputtes Geraet soll ein gelegentlicher
  Vorfall ueber eine lange Spielsitzung sein, keine staendige Plage. Ein
  Shop-Eintrag "Reparatur" repariert alle kaputten Geraete auf einen Schlag,
  Kosten steigen mit der Anzahl.

## Systeme

- **Animationen**: Lauf-Bobbing, Idle-Atmen, ein Stauch-/Streck-"Bump" beim
  Interagieren und ein kurzer Skalierungs-Pop, wenn ein Feld seinen
  Wachstumszustand wechselt.
- **Splitscreen-Coop fuer bis zu 4 Spieler**: eigene Kamera pro Spieler, aber
  gemeinsames Geld, Quests und Shop.
- **Wachstumszyklus**: Pflanzen durchlaufen Keimling -> reif -> ueberreif ->
  (zufaellig) Unkraut. Nur im reifen Zustand lohnt sich die Ernte; zu frueh
  oder zu spaet geerntet bringt nichts, Unkraut kostet Geld beim Entfernen.
  Schon im Keimling-Zustand sieht man, welches Gemuese dort waechst - jede
  Sorte hat ihr eigenes (kleineres, schlichteres) Symbol auch beim Wachsen,
  nicht nur fertig gereift.
- **Shop & Upgrades**: Lauftempo, Wachstumsgeschwindigkeit, Verkaufspreis,
  Drohnen-Tempo, Maeher-Tempo, Unkraut-Resistenz, Scheune und Vogelscheuche
  lassen sich je bis Stufe 60 gegen Gold verbessern (Kosten steigen
  exponentiell, aber gemaessigt genug fuer ein erreichbares Endgame).
- **Tag/Nacht-Zyklus**: Farbueberlagerung dunkelt die Szene nachts ab, Pflanzen
  wachsen in tiefster Nacht bis zu 2,5x langsamer (deutlich spuerbar, nicht nur
  ein leichtes Abbremsen) und die Unkraut-Chance steigt.
- **Quests**: Ein zufaelliges Ziel ist immer aktiv (ernten/pflanzen/verdienen),
  bei Abschluss gibt es eine Gold-Belohnung und ein neues, schwereres Ziel.
- **Wetter**: Alle 20-40 Sekunden wechselt das Wetter zufaellig (Klar ist mit
  Abstand am haeufigsten, damit die anderen vier etwas Besonderes bleiben).
  **Regen** beschleunigt das Wachstum (-35% Wachstumsdauer), bringt aber mehr
  Unkraut (+30% Chance). **Duerre** verlangsamt das Wachstum (+60% Dauer).
  **Sturm** laesst deutlich mehr Voegel auftauchen (+120% Spawn-Chance).
  **Erntesegen** verdoppelt kurzzeitig den Verkaufspreis jeder Ernte. Aktuelles
  Wetter ist im HUD sichtbar (mit Restzeit) und faerbt den Bildschirm leicht
  ein; Regen/Sturm zeigen zusaetzlich fallende Streifen.
- **Grossauftraege**: Alle 60-120 Sekunden erscheint ein zeitlich begrenzter
  Zusatzauftrag fuer eine zufaellige freigeschaltete Gemuesesorte ("Liefere 20x
  Karotten in 75 Sekunden!") mit grosszuegiger Gold-Belohnung, unabhaengig von
  der normalen Endlos-Quest - eigenes Banner oben rechts mit Fortschritts- und
  Countdown-Balken. Nur ein Grossauftrag laeuft gleichzeitig; wird er verpasst,
  verfaellt nur der Bonus, nichts sonst geht verloren.
