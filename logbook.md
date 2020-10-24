# Tagebuch von VADM

15.01.2019      Projektstart
27.01.2019      Das Ausführen von nativem Code im Kind-Prozess funktioniert grundsätzlich, allerdings erscheint die Ausgabe des Beispielprogramms nicht?!
02.02.2019      write() schlägt mit EFAULT (32-Bit ABI, int 80) bzw. EACCES (64-Bit ABI, syscall) fehl, keine Ahnung warum
30.06.2019      Wenn Python ein 64-Bit-Programm ist kann der 32-Bit-Code nicht funktionieren (und umgekehrt) => C-Version ausprobieren
                Oder vielleicht die System Calls im Kind-Prozess mit ptrace() tracen?
13.12.2019      C-Version funktioniert
25.12.2019      Loader von C++ nach C portiert und angepasst
03.01.2020      Erste Instruktion (BCC) übersetzt
06.03.2020      Alle Instruktionen ausser Sprünge (BRA, BCC, JSR) übersetzt
11.04.2020      Sprünge auf Basis von Translation Units und mit einem Cache (wie bei VMware) implementiert
14.04.2020      Übersetzter Code (ohne Systemroutinen) wird ausgeführt
02.05.2020      Programm wird vollständig ausgeführt :-)
11.10.2020      Code wird dynamisch übersetzt
??.??.2020      Artikel fertiggestellt und veröffentlicht
??.??.202?      Programm fertiggestellt (kein ptrace() mehr, Code-Generierung in eine eigene Bibliothek ausgelagert, Warnungen, Unit Tests, TODOs abgearbeitet)
