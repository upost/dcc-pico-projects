# DCC Decoder for Raspberry Pico

Software-Decoder für preisgünstige Lichtsignal-Ansteuerung mit Raspberry Pico

von Uwe Post

https://github.com/upost

# Allgemeines

Der Raspberry Pico ist ein seit 2021 erhältlicher, sehr preisgünstiger Microcontroller (ca. 7-10 Euro).
Er eignet sich aufgrund seiner vielen Ausgänge zur günstigen Ansteuerung mehrerer Modellbahn-Lichtsignale.

Dieses Programm für Raspberry Pico-Microcontroller ermöglicht die Ansteuerung von mehreren
Lichtsignalen via DCC-Protokoll (Accessory Decoder). Hardwareseitig ist eine Spannungsversorgung
mit 3,3 Volt erforderlich. Das DCC-Signal muss über einen Optokoppler ebenfalls mit 3,3 Volt
angelegt werden. Diese erforderliche Vorschaltung (Gleichrichter, Spannungsregler, Optokoppler)
bleibt Ihnen überlassen.

Die Anzahl ansteuerbarer Lichtsignale hängt von der Anzahl der gewünschten Signalbegriffe ab und ist
nur durch die Anzahl der Schaltausgänge des Microcontrollers nach oben begrenzt.

Die Konfiguration der Anschlüsse findet im Code statt, danach muss das Programm gebaut und auf den
Microcontroller kopiert werden. Wie das geht, können Sie der Dokumentation des Raspberry Pico entnehmen.

Dieses Programm ist in C++ verfasst. Programmierkenntnisse sind nicht notwendig, wenn Sie es verwenden möchten.
Befolgen Sie einfach die Anweisungen unter "Konfiguration".

Das Programm liegt als Quellcode vor. Um daraus Binärcode zu machen, der auf die Platine gespielt werden kann,
benötigen Sie eine Buildumgebung mit Visual Studio Code. Bitte konsultieren Sie die Links
in diesem Dokument, um herauszufinden, wie man das macht.


# Beta

Die Software befindet sich im Beta-Stadium. Sie wird derzeit getestet auf der Anlage 
des Autors. Zur Nutzung ist einschlägiges Knowhow erforderlich, da noch keine
Komfortfunktionen vorhanden sind.

# Konfiguration

Die Konfiguration erfolgt in der Datei dcc3signal.cpp.

Der Pin für den DCC-Eingang ist definiert in dieser Zeile:

`#define DCC_IN 21`

An diesen Pin schließen Sie den Ausgang Ihres Optokopplers an.

Die anzuschließenden Signale sind wie folgt definiert:

`Signal signal1(49, 2, SIGNAL_AUSFAHRSIGNAL);`

Jedes Signal hat eine Objektbezeichnung signal1, signal2, ... und so weiter und muss deklariert werden.
In Klammern stehen drei Parameter:
1. DCC-Adresse. Je nach Zentrale müssen Sie hier den "Adresstrick" anwenden: Die tatsächliche, intern verwendete Adresse ist (49-1)/4, also 12. Wenn der Decoder nicht auf die 49 reagiert, versuchen Sie es mit der 12. Dementsprechend müssen Sie hier Adressen eingeben, die ein Vielfaches von 4 sind plus 1. Also 10 entspricht 41, 11 entspricht 45 usw.
2. Ausgangspin für die erste LED des Signals. Die weiteren LEDs folgen aufwärts zählend. Die Anzahl hängt vom Typ des Signals ab.
3. Typ des Signals. Vorgesehen sind derzeit H/V-Lichtsignale: SIGNAL_HAUPTSIGNAL1 (Blocksignal, nur grün und rot), SIGNAL_HAUPTSIGNAL2 (Einfahrsignal, grün, rot, orange), SIGNAL_AUSFAHRSIGNAL (Ausfahrsignal, grün, rot, orange, rot, weiß) Die angegebene Reihenfolge der Signalfarben entspricht jener der LEDs.

Achten Sie darauf, keine Ausgangspins mehrfach zu verwenden. Wenn Ihr signal1 ein Ausfahrsignal ist mit 
ersten Pin 2, benötigt es die Pins 2 (grün), 3 (rot links), 4 (orange), 5 (rot rechts) und
 6 (weiße LEDs des Rangiersignals). Demzufolge müsste das nächste Signal bei Ausgangspin 7 anfangen.

Sie können mehr als die 3 im gegebenen Code verwendeten Signale deklarieren, kopieren Sie dazu einfach die Zeile
und nennen Sie das nächste Signal signal4, signal5 usw. 
In den folgenden Zeilen müssen Sie alle verwendeten Signale aktivieren:
`Device* signals[] = {&signal1,&signal2,&signal3};`
Falls Sie mehr als drei Signale verwenden, schreiben Sie sie einfach mit in die Klammern, vergessen Sie dabei das & nicht.


Hinweis: Der im Code verwendete BUTTON (Pin 27) ist zu Testzwecken eingebaut. Wenn Sie einen Taster an den Pin
anschließen, können Sie damit die Signalbilder manuell durchschalten.


# Build:

Mit VS Code Build drücken

oder

`make -j4`

im richtigen Verzeichnis (build)

# Debugging mit Minicom

Wenn Pico Go sich nicht mit dem Pico verbinden mag, muss Minicom verwendet werden:

 `minicom -b 115200 -o -D /dev/ttyACM0`

 

# Links

https://www.heise.de/developer/artikel/Gutes-Wetter-mit-Raspberry-Pi-Pico-MicroPython-und-Visual-Studio-Code-5066047.html

http://pico-go.net/docs/start/quick/

https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf



