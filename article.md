# VADM2 - Virtual AmigaDOS Machine - Version 2

Ziel ist es, das Programm `loop` (11 unterschiedliche Assembler-Instruktionen, 3 Systemroutinen), das für einen Amiga mit einem Motorola-680x0-Prozessor und Amiga OS geschrieben wurde, mit Hilfe von Binary Translation und API-Emulation auf einem Rechner mit Intel-x86-Prozessor und Linux zum Laufen zu bringen.

CHALLENGE ACCEPTED :-)

Am sinnvollsten wäre es wahrscheinlich, den Disassembler von Musashi (<https://github.com/kstenerud/Musashi/blob/master/m68kdasm.c>) als Basis zu verwenden und das Erzeugen des x86-Codes in den Handlern für die einzelnen Instruktionen zu implementieren. Dann müsste ich den 680x0-Code nicht selber decodieren. Allerdings wäre es dann wahrscheinlich auch sinnvoll, das ganze Programm in C zu schreiben.

Alternative für die Code-Erzeugung: LLVM IR mit Python => http://llvmlite.pydata.org/en/latest/
Dann müsste ich allerdings den Musashi-Disassembler (zumindest den Teil, den ich brauche) nach Python portieren.

Wahrscheinlich bessere Alternative: Keystone => http://www.keystone-engine.org/docs/tutorial.html. Dann könnte ich das Programm entweder in C oder in Python schreiben.


## Register-Mapping
| Register im Motorola 680x0 | Register im Intel x86-64 | Verwendung
| -------------------------- | ------------------------ | ----------
| A0                         | RAX
| A1                         | RCX
| A2                         | RDX
| A3                         | RBX
| A4                         | RDI
| A5                         | RBP                      | Frame Pointer
| A6                         | RSI                      | Basisadressen der Bibliotheken des Amiga OS
| A7                         | RSP                      | Stack Pointer

D0 - D7 => R8D - RD15


## Meilensteine
15.01.2019      Projektstart
27.01.2019      Das Ausführen von nativem Code im Kind-Prozess funktioniert grundsätzlich, allerdings erscheint die Ausgabe des Beispielprogramms nicht?!
02.02.2019      write() schlägt mit EFAULT (32-Bit ABI, int 80) bzw. EACCES (64-Bit ABI, syscall) fehl, keine Ahnung warum
30.06.2019      Wenn Python ein 64-Bit-Programm ist kann der 32-Bit-Code nicht funktionieren (und umgekehrt) => C-Version ausprobieren
                Oder vielleicht die System Calls im Kind-Prozess mit ptrace() tracen?
13.12.2019      C-Version funktioniert
25.12.2019      Loader nach von C++ nach C portiert und angepasst
03.01.2020      Erste Instruktion (BCC) übersetzt


## Links
* <https://github.com/dismantl/linux-injector>
* Artikel, der beschreibt, wie sich fehlende Funktionalität von ptrace auf macOS mit Mach-Nachrichten nachbauen lässt: <http://uninformed.org/index.cgi?v=4&a=3&p=14>
* Codierung der x86-Instruktionen: <http://ref.x86asm.net>, <http://www.c-jump.com/CIS77/CPU/x86/index.html>
* <http://www.jagregory.com/abrash-zen-of-asm/#introduction-pushing-the-envelope>
* <https://en.wikibooks.org/wiki/X86_Assembly>
