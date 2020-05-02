# VADM2 - Virtual AmigaDOS Machine - Version 2

Irgendwann im Frühjahr 2016 hatte ich eine Idee, die mir im ersten Moment ziemlich verrückt erschien. Ich könnte doch einen Emulator schreiben, mit dem man Programme, die für ein anderes Betriebssystem geschrieben wurden, auf einem Mac oder einem Linux-Rechner ausführen.  Ich weiss gar nicht mehr genau, wie ich auf diese Idee gekommen bin. Aber für Betriebssystem und systemnahe Programmierung hatte ich mich ja schon immer interessiert und irgendwie hing es wohl damit zusammen. Am Anfang dachte ich an MS-DOS als zu emulierendes Betriebssystem aber nach einer Weile dachte ich mir, es wäre doch viel cooler, das AmigaOS, das Betriebssystem des Computers meiner Jugend, zu emulieren. Und vielleicht könnte ich es sogar schaffen, Programme, die für den Amiga geschrieben wurden, ohne erneutes übersetzen, also in ihrer binären Form auszuführen?

Dass so etwas grundsätzlich möglich ist war mir klar. Es gibt ja einige Beispiele solcher Emulatoren, zum Beispiel die Virtual DOS Machine in älteren Windows-Versionen (die dann auch als Namensgeber diente), das Wine-Projekt oder auch Wabi von Sun Microsystems, eine Software, die Windows-3.x-Programme auf Sun's Betriebssystem Solaris ausführen konnte. Eine zusätzliche Schwierigkeit bei Amiga-Programmen war allerdings, dass der Amiga ja Prozessoren der 68000er-Familie von Motorola verwendete. Wenn mein Emulator also binärkompatibel (und nicht nur quellkompatibel) sein sollte müsste ich auch den Prozessor emulieren. Auch dafür, dass so etwas möglich ist, gibt es Beispiele. Zum Beispiel war in der Version von Windows NT für den Alpha-Prozessor von DEC ein Emulator enthalten, der Programme ausführen konnte, die für x86-Prozessoren geschrieben waren. So einen Emulator selber zu schreiben erschien mir aber als eine zu grosse Aufgabe, um sie als Hobbyprojekt anzugehen, und ein kurzer Blick in das Datenblatt der 68000er-Prozessoren bestätigte meine Meinung. Diese Prozessoren waren für ihre Zeit eben doch schon ziemlich fortgeschritten und komplex. Nach kurzer Recherche stellte ich allerdings fest, dass es bereits einen Emulator dafür gibt, der einen ganz brauchbaren Eindruck machte, nämlich [Musashi](https://github.com/kstenerud/Musashi). Dieser Emulator wird auch von dem [MAME-Projekt](https://www.mamedev.org/) verwendet.

Mit Hilfe von Musashi schaffte ich es dann tatsächlich, so einen Emulator schreiben. Den Code dazu gibt es [hier](https://github.com/wiemerc/VADM). Obwohl der Emulator funktionierte (und sogar relativ mächtig war, er konnte einen Klon des von Unix und Linux bekannten `find`-Kommandos ausführen) war ich noch nicht ganz damit zufrieden weil er ja nicht vollständig meine eigene Leistung war. Einige Zeit später hörte ich dann zum ersten Mal von [Binary Translation](https://en.wikipedia.org/wiki/Binary_translation). Vielleicht könnte ich ja diese Technik nutzen, um einen Emulator ohne Verwendung von Musashi zu schreiben?

Ziel ist es, das Programm `loop` (10 unterschiedliche Assembler-Instruktionen, 3 Systemroutinen), das für einen Amiga mit einem Motorola-680x0-Prozessor und Amiga OS geschrieben wurde, mit Hilfe von Binary Translation und API-Emulation auf einem Rechner mit Intel-x86-Prozessor und Linux zum Laufen zu bringen.

CHALLENGE ACCEPTED :-)

Am sinnvollsten wäre es wahrscheinlich, den Disassembler von Musashi (<https://github.com/kstenerud/Musashi/blob/master/m68kdasm.c>) als Basis zu verwenden und das Erzeugen des x86-Codes in den Handlern für die einzelnen Instruktionen zu implementieren. Dann müsste ich den 680x0-Code nicht selber decodieren. Allerdings wäre es dann wahrscheinlich auch sinnvoll, das ganze Programm in C zu schreiben.

Alternative für die Code-Erzeugung: LLVM IR mit Python => http://llvmlite.pydata.org/en/latest/
Dann müsste ich allerdings den Musashi-Disassembler (zumindest den Teil, den ich brauche) nach Python portieren.

Wahrscheinlich bessere Alternative: Keystone => http://www.keystone-engine.org/docs/tutorial.html. Dann könnte ich das Programm entweder in C oder in Python schreiben.


## Laden des Programms

Das AmigaOS verwendete das sogenannte [Hunk-Format](TODO) für ausführbare Programme. Eine genaue Beschreibung dieses Formats findet sich im _AmigaDOS Manual_ oder auch im _Amiga-Guru-Buch_. Grob gesagt besteht eine Programmdatei in diesem Format aus mehreren Hunks, die den Segmenten bei anderen Formaten wie ELF oder PE entsprechen. Normalerweise besteht eine Programmdatei aus drei Hunks, je einem für Code, Daten und BSS. Optional gibt es noch einen Hunk mit den Debugging-Informationen. Diese Hunks sind dann nochmal in mehrere Blöcke unterteilt, und zwar in einen Block mit dem Code beziehungsweide den Daten (in dem BSS-Hunk ist dieser natürlich leer), einen Block mit [Verschiebeinformationen](TODO) und einen Block mit den Symbolen für diesen Hunk (wenn sie nicht vom Linker aus der Programmdatei gelöscht wurden). Verschiebeinformationen sind deshalb notwendig weil das AmigaOS keinen virtuellen Speicher verwendete und deshalb beim Linken eines Programms die absolute Adresse, an der das Programm geladen wird, nicht bekannt war.

Das Lesen der Programmdatei und das Laden des Codes und der Daten in den Speicher ist in der Routine `load_program` in der Datei `loader.c` implementiert und ebenso das Anpassen der Adressen auf Basis der Verschiebeinformationen. Dabei gibt es ein interessantes Detail. Auf dem Amiga waren alle Adressen 32 Bits breit und demzufolge natürlich auch die anzupassenden Adressen. Wie sollte ich also die 64 Bits der Adressen, an die die Hunks geladen werden, in den zur Verfügung stehenden 32 Bits unterbringen? Diese Problem habe ich so gelöst, dass die Hunks mit `mmap` an feste Adressen unterhalb der 4GB-Grenze geladen werden und so 32-Bit-Adressen entstehen (die oberen 32 Bit sind eben alle 0).

## Übersetzen des Codes

Der Code kann nicht in einem Rutsch übersetzt werden weil die Sprungziele aufgrund der unterschiedlichen Längen der Instruktionen bei 680x0 und x86 nicht bekannt sind (Offsets / absolute Adressen sind unterschiedlich). 

Neuer Ansatz:
* Übersetzen von einzelnen [Basic Blocks](https://en.wikipedia.org/wiki/Basic_block) und Ablage des übersetzten Codes in einzelnen Speicherblöcken
* Jeder Sprungbefehl erzeugt einen neuen Block, Sprungziel = Adresse des Blocks
* Blöcke werden auf Stack abgelegt und rekursiv übersetzt (weil in einem Block auf dem Stack wieder ein Sprungbefehl vorkommen kann)
* Der gleiche Ansatz könnte auch zum dynamischen Übersetzen (zur Laufzeit) verwendet werden => Verzweigungen legen die Adresse des zu übersetzenden Codes auf den Stack und rufen Funktion auf, die Interrupt erzeugt => Kontrolle wird an den Monitor-Prozess zurückgegeben, der den Code übersetzt und die Verzweigung patcht, so dass der übersetzte Code aufgerufen wird (so ähnlich wie bei Overlays).

Bibliothek mit Listen und anderen Datenstrukturen: https://jlk.fjfi.cvut.cz/arch/manpages/listing/extra/libbsd/


## Emulation der Systemroutinen

Jede Bibliothek (auch Exec) wird durch eine separate Shared Library implementiert. Diese Bibliotheken werden vom Kindprozess mit `dlopen` geladen (damit sie im Adressraum des Kindprozesses zur Verfügung stehen). Jede Bibliothek hat ein Funktion zum Erzeugen der Sprungtabelle. Diese Sprungtabelle besteht im Normalfall aus den Instruktionen `INT x` und `RET` für jede Routine. Nur für die implementierten Routinen wird ein `JMP` zur eigentlichen Routine durchgeführt. Der Interrupt für die nichtimplementierten Routinen wird vom Supervisor-Prozess behandelt.


## Register-Mapping

Ungewöhnliche Reihenfolge bei Intel kommt daher, dass die Register (aus welchen Gründen auch immer) so durchnummeriert sind, A5 und A7 werden nach ihrer Verwendung auf EBP und ESP abgebildet.
| Register im Motorola 680x0 | Register im Intel x86-64 | Verwendung
| -------------------------- | ------------------------ | ----------
| A0                         | EAX
| A1                         | ECX
| A2                         | EDX
| A3                         | EBX
| A4                         | EDI
| A5                         | EBP                      | Frame Pointer
| A6                         | ESI                      | Basisadressen der Bibliotheken des Amiga OS
| A7                         | ESP                      | Stack Pointer

D0 - D7 => R8D - RD15


## Meilensteine

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


## Links

* <https://github.com/dismantl/linux-injector>
* Artikel, der beschreibt, wie sich fehlende Funktionalität von ptrace auf macOS mit Mach-Nachrichten nachbauen lässt: <http://uninformed.org/index.cgi?v=4&a=3&p=14>
* Codierung der x86-Instruktionen: <http://ref.x86asm.net>, <http://www.c-jump.com/CIS77/CPU/x86/index.html>
* <http://www.jagregory.com/abrash-zen-of-asm/#introduction-pushing-the-envelope>
* <https://en.wikibooks.org/wiki/X86_Assembly>
* Binary Tranlation in VMware: https://www.vmware.com/pdf/asplos235_adams.pdf
* https://www.nxp.com/files-static/archives/doc/ref_manual/M68000PRM.pdf
* http://x86asm.net/articles/x86-64-tour-of-intel-manuals/
* https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-instruction-set-reference-manual-325383.pdf
