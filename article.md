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

Nach dem Laden des Programms muss der Programmcode übersetzt werden. Das bedeutet, dass für jede der Instruktionen für den 680x0-Prozessor eine (oder auch mehrere) entsprechende Instruktion(en) für den x86-Prozessor erzeugt werden muss (müssen). Dabei bin ich in zwei Schritten vorgegangen. Im ersten Schritt wird die bestehende Instruktion decodiert und in ihre Bestandteile, nämlich [Opcode](https://en.wikipedia.org/wiki/Opcode), [Adressierungsart](https://en.wikipedia.org/wiki/Addressing_mode) und Operanden, zerlegt. Das ist im Prinzip auch das, was ein Prozessor tun muss bevor er eine Instruktion ausführen kann. Im zweiten Schritt wird aus diesen Bestandteilen eine neue Instruktion für den x86-Prozessor erzeugt und entsprechend codiert. Den ersten Schritt habe ich nicht vollständig selber implementiert sondern ich habe dafür Teile des Dissassemblers von Musashi verwendet. Ein Dissassembler muss nämlich die Instruktionen genauso decodieren und zerlegen, nur dass er dann die Bestandteile in lesbarer Form ausgibt. Der Dissassembler von Musashi ist so geschrieben, dass er aus dem Code eine Folge von Instruktionen erzeugt und dann für jede Instruktion mit Hilfe einer Lookup-Tabelle und dem Opcode als Index einen sogeannten Handler aufruft (wer an den Details interessiert ist kann sich die Routine _translate\_unit_ in der Datei `translate.c` zu Gemüte führen). Der nachfolgende Code zeigt den Handler für die Instruktion _move_.

    :::c
    static int m68k_move(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
    {
        uint8_t  src_mode_reg = m68k_opcode & 0x003f;
        uint8_t  dst_mode_reg = (m68k_opcode & 0x0fc0) >> 6;
        Operand  srcop, dstop;
        int      nbytes_used = 0;

        DEBUG("translating instruction MOVE");
        if ((m68k_opcode & 0x3000) != 0x2000) {
            ERROR("only long operation supported");
            return -1;
        }

        nbytes_used += extract_operand(src_mode_reg, inpos, &srcop);
        // destination operand has mode and register parts swapped
        dst_mode_reg = ((dst_mode_reg & 0x07) << 3) | ((dst_mode_reg & 0x38) >> 3);
        nbytes_used += extract_operand(dst_mode_reg, inpos, &dstop);

        // call the appropriate function depending on the combination of source / destination operand type
        if ((srcop.op_type      == OP_MEM)  && (dstop.op_type == OP_DREG))
            x86_encode_move_mem_to_dreg(srcop.op_value, dstop.op_value, outpos);
        else if ((srcop.op_type == OP_IMM)  && (dstop.op_type == OP_DREG))
            x86_encode_move_imm_to_dreg(srcop.op_value, dstop.op_value, outpos);
        else if ((srcop.op_type == OP_DREG) && (dstop.op_type == OP_MEM))
            x86_encode_move_dreg_to_mem(srcop.op_value, dstop.op_value, outpos);
        else if ((srcop.op_type == OP_DREG) && (dstop.op_type == OP_DREG))
            x86_encode_move_dreg_to_dreg(srcop.op_value, dstop.op_value, outpos);
        else {
            ERROR("combination of source / destination operand types %d / %d not supported", srcop.op_type, dstop.op_type);
            return -1;
        }
        return nbytes_used;
    }

Schwieriger wird es bei den Sprungbefehlen (BCC). Bei diesen Befehlen ist der Operand die Adresse im Code (absolut oder relativ), an die gesprungen werden soll (oder auch ein Offset, der zu einem Adressregister addiert wird, was dann wiederum eine Adresse ergibt). Diese Adresse, also das Sprungziel, ist aber für den übersetzten Code beim Übersetzen der Sprungbefehle noch gar nicht bekannt und *nicht* gleich der Adresse im Orginalcode. Sowohl absolute als auch relative Adressen und Offsets ändern sich beim Übersetzen. Das liegt zum einen daran, dass der übersetzte Code natürlich im Speicher an einer anderen Stelle liegt als der Orginalcode. Zum anderen sind im Allgemeinen funktional identische Instruktionen bei Motorola und Intel unterschiedlich lang, bestehen also aus einer unterschiedlichen Anzahl von Bytes. Somit unterscheidet sich das Layout des Codes im Speicher zwischen Motorola und Intel. Dieses Problem habe ich so gelöst, dass ich nicht das komplette Programm "in einem Rutsch" übersetze. Stattdessen unterteile ich das Programm in Blöcke, sogenannte _Translation Units_, und übersetze diese einzeln. Die übersetzten Blöcke werden in einem _Translation Cache_ abgelegt. Die Methode stammt nicht von mir sondern aus [diesem](https://www.vmware.com/pdf/asplos235_adams.pdf) Paper, das beschreibt, wie in VMware Binary Translation durchgeführt wird. So funktioniert diese Methode im Detail:

* Die Routine _translate\_unit_ wird mit der Adresse des zu übersetzenden Code-Blocks = Translation Unit aufgerufen. Der erste Code-Block startet natürlich am Anfang des Programms.
* _translate\_unit_ überprüft, ob die Translation Unit bereits übersetzt wurde und sich im Translation Cache befindet. Dieser Cache speichert die Zuordnung von Adressen im Originalcode zu Adressen im übersetzten Code für alle bereits übersetzten Translation Units (implementiert als binärer Suchbaum, die Routinen _tc\_get\_addr_ _tc\_put\_addr_ in der Datei `tlcache.c`). Befindet sich die Translation Unit im Cache gibt es nichts zu tun und _translate\_unit_ gibt einfach die Adresse des übersetzten Codes im Cache zurück.
* Befindet sich die Translation Unit noch nicht im Cache muss der Code übersetzt werden. Dazu wird zuerst ein Speicherblock allokiert, in dem dann übersetzte Code abgelegt wird. Diese Blöcke sind der Einfachheit halber immer 4 KB (eine volle Speicherseite) gross und der Speicher muss logischerweise ausführbar sein. Die Adresse des Speicherblocks wird zusammen mit der Adresse der Translation Unit im Translation Cache gespeichert.
* Wie schon oben beschrieben wird dann die Translation Unit Instruktion für Instruktion übersetzt, und zwar solange bis eine terminierende (TODO: Begriff?) Instruktion erreicht ist. Das sind Instruktionen, die eine Translation Unit abschliessen. Nachdem ich ja nur einen kleinen Teil der Instruktionen implementiert habe sind das bei VADM die Instruktionen BCC (Sprünge) und RTS (Rücksprung aus einer Routine).
* Handelt es sich bei der terminierenden Instruktion um einen Sprungbefehl passiert folgendes.
* _translate\_unit_ wird zweimal rekursiv aufgerufen, einmal mit dem Sprungziel als Adresse, einmal mit der Adresse der Instruktion nach dem Sprungbefehl. Damit entstehen zwei neue Translation Units.
* Der Sprungbefehl wird mit der Adresse der ersten Translation Unit als Sprungziel übersetzt. Zusätzlich wird nach dem Sprungbefehl ein unbedingter Sprung (JMP) eingefügt, mit dem zur zweiten Translation Unit gesprungen wird. Das bedeutet, dass die aktuelle Translation Unit mit diesen beiden neuen Translation Units verknüpft wird und die Programmausführung wird mit einer dieser beiden Units fortgesetzt (mit der ersten wenn die Sprungbedingung erfüllt ist, ansonsten mit der zweiten).

Die Translation Units stellen in den meisten Fällen sogenannte [Basic Blocks](https://en.wikipedia.org/wiki/Basic_block) dar. Allerdings kann es bei der beschriebenen Methode vorkommen, dass Basic Blocks entstehen, die nicht nur die erste Instruktion als "Eingang" haben sondern auch noch andere Instruktionen, so zum Beispiel bei der zweiten Translation Unit, was der strikten Definition widerspricht.

Der gleiche Ansatz könnte auch zum dynamischen Übersetzen (zur Laufzeit) verwendet werden. Bei einem Sprungbefehl würden die beiden nachfolgenden TUs noch nicht gleich übersetzt werden sondern _translate\_unit_ würde erstmal nur Speicherblöcke für die TUs allokieren und in diesen eine Interrupt-Instruktion und die Adresse des zu übersetzenden Codes ablegen. Beim Sprung in eine dieser TUs würde der Interrupt die Kontrolle an den Supervisor-Prozess zurückgeben (analog zu einem Page Fault im Betriebssystem könnte man das einen "Branch Fault" nennen). Dieser würde erneut _translate\_unit_ erneut aufrufen, um die entsprechende TU zu übersetzen und dann die Programmausführung mit dieser TU fortsetzen. Das würde im Prinzip so ähnlich funktionieren wie bei einem Programm mit [Overlays](https://en.wikipedia.org/wiki/Overlay_(programming)).

TODO: Register-Mapping
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

D0 - D7 => R8D - R15D


## Emulation der Systemroutinen

Die zweite Aufgabe des Emulators ist es, die von dem Beispielprogramm verwendeten Systemroutinen zu emulieren. Dabei handelt es sich um die Routinen _OpenLibrary_, _PutStr_ und _CloseLibrary_. Die Herausforderung war hierbei nicht, die eigentliche Funktionalität nachzubauen sondern, die Aufrufe dieser Routinen im Beispielprogramm abzufangen und auf die entsprechenden Routinen im Emulator "umzubiegen". Um diesen Teil verstehen zu können muss ich zuerst erklären, wie der Aufruf von Systemroutinen im AmigaOS funktionierte. Im AmigaOS waren die Systemroutinen nach Themen in verschiedenen Bibliotheken (_Exec_, _DOS_, _Intuition_ usw.) zusammengefasst. Jede dieser Bibliotheken bestand neben den eigentlichen Routinen aus einer Sprungtabelle (und einer Struktur mit verschiedenen Verwaltungsinformationen für die Bibliothek, die aber für die Emulation keine Rolle spielt). Diese Tabelle befand sich unterhalb der Basisadresse der Bibliothek (die Adresse, die von _OpenLibrary_ zurückgegeben wurde) und bestand aus absoluten Sprüngen (die Instruktion _JMP_) zu den jeweiligen Routinen. Die Offsets in dieser Tabelle stellten die öffentliche API der Bibliothek dar (beschrieben in den sogenannten FD-Dateien) und eine Routine einer Bibliothek wurde durch Laden der Basisadresse in das Register A6 und der Instruktion `JSR -<Offset der Routine>(A6)` aufgerufen. 

Wie kann nun der Emulator so einen Aufruf erkennen und stattdessen eine Routine im Emulator aufrufen? Ich habe das so gelöst, dass sich die Systemroutinen nicht im Emulator selber sondern in Shared Libraries befinden (im Unterverzeichnis libs, die Bibliothek `libexec.so` enthält die Routinen _OpenLibrary_ und _CloseLibrary_, `libdos.so` enthält _PutStr_). `libexec.so` wird vor dem Starten des Amiga-Programms geladen (mit `dlopen`) und die Basisadresse an der Adresse 4 abgelegt (die einzige fest Adresse im AmigaOS), und zwar im Kindprozess, damit sie im Adressraum des Kindprozesses zur Verfügung steht. Das Laden von `libdos.so` erfolgt durch den Aufruf von _OpenLibrary_ im Amiga-Programm. Zusätzlich zu den erwähnten Routinen enthalten beide Bibliotheken auch noch eine Routine zum Erzeugen der gerade beschriebenen Sprungtabelle. Allerdings sieht diese Tabelle etwas anders aus als die Sprungtabellen im AmigaOS, und das nicht nur weil sie natürlich aus Instruktionen für Intel-Prozessoren besteht. Tatsächlich werden sogar zwei Tabellen erzeugt, aber zu der zweiten Tabelle komme ich gleich. Für den Emulator ist der Normalfall nämlich, dass eine Routine _nicht_ implementiert ist (weil ich ja nur drei von mehreren Hundert Routinen implementiert habe). In diesem Fall besteht der Eintrag in der Tabelle für die entsprechende Routine nur aus den Instruktionen `INT TODO` und `RET`. Das bedeutet, dass beim Aufruf einer nichtimplementierten Routine ein Interrupt erzeugt wird, den der Supervisor-Prozess behandelt, indem er das Amiga-Programm mit einer Fehlermeldung beendet.

Interessanter ist es natürlich wenn ein Routine implementiert ist. Das nachfolgende Bild zeigt, was in diesem Fall passiert.

TODO: Bild für Aufruf einer Routine
TODO: Beschreibung des Ablaufs

There are two jump tables to create. The first is the one that is used by the programs
that use the library to call the functions. The offsets in this table are specified in
the API documentation of the AmigaOS (in the FD files). They have to be subtracted from
the library base address as returned by OpenLibrary(). This means, we place this table
at the end of the memory block reserved for the jump tables and have OpenLibrary() return
this address.
In the AmigaOS, this table contained absolute jumps to the actual functions. However,
as the entries in this table are only 6 bytes apart each, there is not enough room to put
absolute jumps to the functions with 64-bit addresses there. Therefore, we create a
second table with the absolute jumps (and some additional code, so it's actually a thunk)
and put relative jumps with 32-bit offsets to the second one into the first one (5 bytes
in x86-64 code). This second tables lives at the start of the memory block. For the functions
that are not implemented, the first table contains interrupt instructions to inform the
supervisor process that an unimplemented function has been called by the program.


## Entwicklung

* Unit-Tests
* GDB mit [Pwndbg](https://github.com/pwndbg/pwndbg)


## Ausblick
TODO: Alternativen zum Erzeugen des x86-Codes: LLVM, Keystone, AsmJit
TODO: Code-Genierung über eine Tabelle / Mini-DSL


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
* weitere Bibliothek zum Erzeugen von Maschinencode: https://asmjit.com/
