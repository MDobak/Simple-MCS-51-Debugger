P�tora roku temu napisa�em w C (wym�g wyk�adowcy) taki program na studiach (na pierwszym semestrze ;) )- jest to symulator mikrokontroler�w MCS-51. Program wydaje si� by� poprawny, bez problemu radzi sobie z nawet bardzo z�o�onymi programami (w przyk�adowych programach jest nawet tetris :D ), ma kilka ciekawych funkcji debuggerskich, wbudowany deassambler, mo�liwo�� dodawania breakpoint�w czy nawet breakpont�w z warunkami i szczerze m�wi�c nie pami�tam co on wi�cej mo�e, bo do tego kodu nie dotyka�em si� dobre 1.5 roku. Po oddaniu programu na zaliczenie co� jeszcze w nim grzeba�em st�d par� niedoko�czonych funkcji. Pozby�em si� te� niestety wi�kszo�ci komentarzy bo wiele z nich by�o ju� aktualnych i raczej wprowadza�o w b��d ni� pomaga�o ;( 

Programu rozwija� raczej nie b�d�, ale, �e jest to kawa�ek ca�kiem przyzwoitego kodu i szkoda �eby si� marnowa� oddaje go wam, mo�e komu� si� przyda by lepiej zrozumie� dzia�anie mikrokontrolera, mo�e kto� zacznie go rozwija� a mo�e kto� wykorzysta fragment kodu ;) 

I to tyle, niech program i jego �r�d�a m�wi� dalej za siebie. 

Poni�szy poradnik jest cz�sciowo nieaktualny!

WST�P
========================

Witaj w programie S51D! Ten program jest rozpowszechnainy na licencji
GNU General Public License (GPL). Teraz...

WPROWADZENIE
========================

S51D jest symulatorem 8-mio bitowych mikrokontroler�w opertych na
architekturze MCS-51. Program by� tworzony g��wnie z my�l� o mikrokontrolerze
Intel 8052, co oznacza, �e jest tak�e kompatybilny z Intel 8031/8051 oraz
innymi kt�re s� kompatybilne z wymienionymi. 

Program zak�ada, �e dysponujemy 256 bajtami pami�ci wewn�trznej RAM,
64kB pami�ci zewn�trznej ROM i 64kB pami�ci zewn�trznej RAM co stanowi 
maksymlne dopuszczalne warto�ci. Program obs�uguje wszystkie instrukcje
procesora, instrukcje 0xA5 traktuje jak nop. Obs�ugiwane s� wszystkie timery
i przerwania. 

Program wyposa�ony jest w debugger pozwalaj�cy kontrolowa� dzia�anie
wgranego do pami�ci programu, wp�ywa� na jego prace, modyfikowa� pami��
ustawia� breakpointy aktywowane na trzy r�ne sposoby: aktywowane ustawieniem
PC (program counter, lub te� IP - instruction pointer) na okre�lon� warto��,
pr�b� odczytu okre�lonego adresu z wybranej pami�ci lub zmian� warto�ci
wybranych adres�w w pami�ci.

INSTRUKCJA
========================

                               ...Podstawy...

Po uruchomieniu programu pojawia si� znak zach�ty, kt�ry zazwyczaj wygl�da tak:
cmd:>

Po nim, nale�y poda� jedn� z obs�ugiwanych przez program komend. Ich pe�n�
list� mo�na zobaczy� wpisuj�c komend� `help`.

Po uruchomieniu programu pami�� mikrokontrolera jest wyzerowana, mo�na si�
o tym przekona� wpisuj�c komende `hex rom` kt�ra spowoduje wy�wietlenie
zawarto�ci pami�ci rom. 
cmd:>hex rom
0x0000 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
0x0010 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
0x0020 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
0x0030 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
...
0xFFC0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
0xFFD0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
0xFFE0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
0xFFF0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................

Wy�wietlony listing jest bardzo d�ugi i zazwyczaj nie mie�ci si� w oknie
konsoli. Mo�na wi�c zapisa� go do pliku zmianiaj�c standardowe wyj�cie 
operatorem `>`.
cmd:>hex rom > pamiec_rom.txt

Po komendzie `hex` nale�y poda� nazwe pami�ci do wy�wietlenia, np. "rom", 
"intram", "extram", "sfr". Po nazwie mo�na poda� adres od kt�rego ma zacz��
by� wyswietlana zawarto�� pami�ci i adres ko�cowy. Np:
cmd:>hex sfr 90 b0
0x0090 FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
0x00A0 FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
0x00B0 FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................

Aby za�adowa� do pami�ci program nale�y u�y� komendy `load` i poda� lokalizacje
pliku intel hex z kodem programu. Np:
cmd:>load examples/hello.hex

Wykonanie tej komendy nie powoduje wy�wietlenia ��dnego wyniku je�li
program zosta� wczytany do pami�ci bez problemu. Argumenty do komend
mo�na podawa� w cudzys�owach dzi�ki czemu mo�na w nich zawiera� spacje, inn�
metod� wy��czenia traktowania bia�ych znak�w, jako separator�w argument�w jest
poprzedzenie ich znakiem "\" (lewy uko�nik). Analogicznie mo�na poprzedzi�
cudzys�owy tym znakiem aby nie traktowa� ich specjalne. Lewym uko�nikiem
mo�na te� poprzedzi� sam uko�nik aby go doda� do argumentu. Przyk�ady:
cmd:>load "../rozne pliki/moj plik.hex"
cmd:>load ../rozne\ pliki/moj\ plik.hex
cmd:>load "..\\rozne pliki\\moj plik.hex"
cmd:>load ..\\rozne\ pliki\\moj\ plik.hex

Mo�emy przekona� si�, �e program zosta� za�adowany ponownie wykonuj�c
komende `hex`:
cmd:>hex code 6c0 700
0x06C0 9B F5 83 22 20 F7 14 30 F6 14 88 83 A8 82 20 F5 ..." ..0...... .
0x06D0 07 E6 A8 83 75 83 00 22 E2 80 F7 E4 93 22 E0 22 ....u.."....."."
0x06E0 75 82 00 22 48 65 6C 6C 6F 20 77 6F 72 6C 64 21 u.."Hello world!
0x06F0 00 3C 4E 4F 20 46 4C 4F 41 54 3E 00 00 00 00 00 .<NO FLOAT>.....
0x0700 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................

Do kontroli dzia�ania wczytanego programu u�ywamy przedewszystkim
komend `run`, `stop`, `output`, `hexoutput`, `step`, `trace`.

Na pocz�tek om�wimy komende `step`. Nie przyjmuje ona �adnych argument�w.
Jej dzia�anie jest bardzo proste. powoduje ona wykonanie jednej instrukcji
procesora, wypisanie na ekran informacji o wykonanej komendzie i niekt�re
warto�ci. Adresy w pami�ciach IDATA, XDATA i SFR s� okre�lane zmieniane w
mom�ncie zmiany ich warto�ci.
cmd:>step
0008: 75 81 3A mov   SP, #3Ah
IDATA[00h]=00   XDATA[0000h]=00 SFR[SP]=3A      PSW=01          TCON=00
ACC=00          DPTR=0000       @DPTR=0000      SBUF[out]=00    SBUF[in]=00

U�ywaj�c komend `byte` i `bit` mo�na wy�wietli�, lub zmieni� inne wybrane
kom�rki pami�ci:
cmd:>byte sfr p0
SFR[80]=FF
cmd:>bit sfr p0 1
SFR[80.1] = true
cmd:>bit sfr p0 1 0
cmd:>byte sfr p0
SFR[80]=FD

Kolejn� komend� pozwalaj�c� wykona� kod jest `trace`. Komenda ta wy�wietla
wykonywane instrukcje. Komenda przyjmuje conajmniej jeden argument okre�laj�cy
ilo�� komend jakie maj� byc wykonane.
cmd:>trace 10
000B: 12 06 E0 lcall 06E0
06E0: 75 82 00 mov   DPL, #00h
06E3: 22       ret
000E: E5 82    mov   A, DPL
0010: 60 03    jz    0015h
0015: 79 00    mov   R1, #00h
0017: E9       mov   A, R1
0018: 44 00    orl   A, #00h
001A: 60 1B    jz    0037h
0037: E4       clr   A
Stoped at 0038h after execute 10 instructions.

Podaj�c dodatkowy parametr "1" funkcja wy�wietla wi�cej informacji. Z tym
parametrem jej dzia�anie przypomina wykonywanie kolejno instrukcji step.
cmd:>trace 3 1
0038: 78 FF    mov   R0, #FFh
IDATA[00h]=FF   XDATA[0000h]=00 SFR[SP]=3A      PSW=01          TCON=00
ACC=00          DPTR=0000       @DPTR=0000      SBUF[out]=00    SBUF[in]=00
003A: F6       mov   @R0, A
IDATA[00h]=FF   XDATA[0000h]=00 SFR[SP]=3A      PSW=01          TCON=00
ACC=00          DPTR=0000       @DPTR=0000      SBUF[out]=00    SBUF[in]=00
003B: D8 FD    djnz  R0, 003Ah
IDATA[00h]=FE   XDATA[0000h]=00 SFR[SP]=3A      PSW=01          TCON=00
ACC=00          DPTR=0000       @DPTR=0000      SBUF[out]=00    SBUF[in]=00
Stoped at 003Ah after execute 3 instructions.

W ka�dej chwili mo�na wy�wietli� adres kolejnej instrukcji jaka zotanie
wykonana, czyli aktualny PC. Aby to zrobi� nale�y u�y� komendy `pc`.
cmd:>pc
PC = 003A

Podaj�c argument do funkcji mo�na zmieni� aktualn� warto�� PC.

Komenda `run` uruchamia program w tle. W trakcie, gdy jest on uruchomiony
niekt�re komendy moga nie dzia�a� (zostanie wy�wietlony komunikat), 
jednak ca�y czas mo�emy analizowa� dzia��nie programu i zatrzyma� go w 
dowolnym momencie komend� `stop`. Program, mo�e sam zatrzyma� wykonywanie
programu gdy mikrokontroler wy�le sygna� PD (power down), natrafi na p�tle
z kt�rej nie mo�e si� wydosta� przyjednoczesnym braku zezwole� dla przerwa� lub
po ukatywnieniu jednego z breakpoint'�w.

W ka�dej chwili mo�na zresetowa� mikrokontroler komedn� `reset` kt�ra przywraca
mikrokontroler do stanu pocz�tkowego. Zeruje pami�ci z wyj�tkiem pami�ci ROM.

Ostatnimi komendami s� output i hexoutput. Powoduj� one uruchomienie 
programu wraz z sukcesywnym wysy�aniem zawarto�ci SBUF i pobieraniem
kodu klawiszy z klawiatury. Dzia��nie programu mo�na zatrzyma� w dowolnym
mom�cie kombinacj� klawiszy Ctrl+Q (mo�na zmieni� komend� `setexitkey`).
cmd:>load hello.hex
cmd:>terminal
Hello world!

W ka�dym momencie mo�na wywo�a� komende `state` kt�ra wy�wietla podstawowe
informacje o mikrokontrolerze.
cmd:>state
Cycles: 2116
Oscillator clocks: 25392
Oscillator frequency: 11059200 Hz
Executed instructions: 1312
Run time: 0.00229384 seconds
Sim time: 0.00200000 seconds
Max value of stack pointer: 0x51
Highest used ROM address: 0x06F0
Highest used IDATA address: 0xFF
Lowest used IDATA address: 0x00
Highest used XDATA address: Not used.
Lowest used XDATA address: Not used.
Microcontroller state: OK

Ostatni� rzecz� o jakiej warto wspomnie� w instrukcji jest disassembler.
Do jego wywo�ania s�u�� funkcje `disassemble` i jej prostszy zamiennik `deasm`.
Komenda przyjmuje dwa argumenty, adres od kt�rego wy�wietla kod i ilo��
istrukcji do wy�wietlenia.
cmd:>deasm 0 10
0000: 02 00 08 ljmp  0008
0003: 12 00 B7 lcall 00B7
0006: 80 FE    sjmp  0006h
0008: 75 81 3A mov   SP, #3Ah
000B: 12 06 E0 lcall 06E0
000E: E5 82    mov   A, DPL
0010: 60 03    jz    0015h
0012: 02 00 03 ljmp  0003
0015: 79 00    mov   R1, #00h
0017: E9       mov   A, R1

Skoro dotar�e� tak daleko w czytaniu tego mam ma�� niespodziank� :-) !
Wykonaj t� komend�:
cmd:>exec for.fun

                            ...Breakpoint'y...

�aden porz�dny debugger nie obejdzie si� bez mo�liwo�ci stawiania breakpoint�w.
Aby utworzy� breakpoint nalezy u�yc komendy `breakpoint` lub jej kr�tszego
zamiennika `break`. Jej u�ycie jest bardzo proste. Bo argumencie podajemy 
adresy w pami�cy rom przy kt�rych dzia�anie programu ma zosta� przerwane.
cmd:>break 041b
cmd:>run
cmd:>pc
PC = 041B
                           ...Pauzy warunkowe...

S51D obs�uguje dwa rodzaje pauz warunkowych, `conditionPause` i jej
prostszy zamiennik `cond`, i `accessPause` z zamiennikiem `access`.

`conditionPause` zatrzymuje dzia�anie kiedy warto�� w pami�ci ram lub
sfr spe�nia okre�lony warunek. Sk�adnia komendy wygl�da tak:
conditionPause mem value operator addr ...
gdzie mem okre�la rodzaj pami�ci, value jest warto�ci� wzgl�dem kt�rej
okre�lone addresy (addr) maj� by� sprawdzone u�ywaj�c operatora.

Poni�szy przyk�ad zatrzyma wykonywanie programu, kiedy pierwszy bit w kom�rce
pod adresem 0xFD w pami�ci zewn�trznej RAM b�dzie w stanie wysokim:
cmd:>load sieve.hex
cmd:>cond xdata 1 and fd
cmd:>terminal
10 iterations

cmd:>hex xdata f0 ff
0x00F0 01 01 01 01 01 01 01 01 01 01 01 01 01 01 00 00 ................

`accessPause` zatrzymuje dzia��nie kiedy program spr�buje odczytwa�, lub
zapisa� co� do okre�lonych kom�rek w pami�ci. W przypadku podania pami�ci
rom, program nie zatrzyma si� przy pr�bie odczytwania warto�ci przez
interpreter instrukcji, do tego s�u�� breakpointy!
cmd:>load hello.hex
cmd:>access code 06ec
cmd:>terminal
Hello wo
cmd:>hex code 6e0 6ef
0x06E0 75 82 00 22 48 65 6C 6C 6F 20 77 6F 72 6C 64 21 u.."Hello world!
