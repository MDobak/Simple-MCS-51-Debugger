Pó³tora roku temu napisa³em w C (wymóg wyk³adowcy) taki program na studiach (na pierwszym semestrze ;) )- jest to symulator mikrokontrolerów MCS-51. Program wydaje siê byæ poprawny, bez problemu radzi sobie z nawet bardzo z³o¿onymi programami (w przyk³adowych programach jest nawet tetris :D ), ma kilka ciekawych funkcji debuggerskich, wbudowany deassambler, mo¿liwoœæ dodawania breakpointów czy nawet breakpontów z warunkami i szczerze mówi¹c nie pamiêtam co on wiêcej mo¿e, bo do tego kodu nie dotyka³em siê dobre 1.5 roku. Po oddaniu programu na zaliczenie coœ jeszcze w nim grzeba³em st¹d parê niedokoñczonych funkcji. Pozby³em siê te¿ niestety wiêkszoœci komentarzy bo wiele z nich by³o ju¿ aktualnych i raczej wprowadza³o w b³¹d ni¿ pomaga³o ;( 

Programu rozwijaæ raczej nie bêdê, ale, ¿e jest to kawa³ek ca³kiem przyzwoitego kodu i szkoda ¿eby siê marnowa³ oddaje go wam, mo¿e komuœ siê przyda by lepiej zrozumieæ dzia³anie mikrokontrolera, mo¿e ktoœ zacznie go rozwijaæ a mo¿e ktoœ wykorzysta fragment kodu ;) 

I to tyle, niech program i jego Ÿród³a mówi¹ dalej za siebie. 

Poni¿szy poradnik jest czêsciowo nieaktualny!

WSTÊP
========================

Witaj w programie S51D! Ten program jest rozpowszechnainy na licencji
GNU General Public License (GPL). Teraz...

WPROWADZENIE
========================

S51D jest symulatorem 8-mio bitowych mikrokontrolerów opertych na
architekturze MCS-51. Program by³ tworzony g³ównie z myœl¹ o mikrokontrolerze
Intel 8052, co oznacza, ¿e jest tak¿e kompatybilny z Intel 8031/8051 oraz
innymi które s¹ kompatybilne z wymienionymi. 

Program zak³ada, ¿e dysponujemy 256 bajtami pamiêci wewnêtrznej RAM,
64kB pamiêci zewnêtrznej ROM i 64kB pamiêci zewnêtrznej RAM co stanowi 
maksymlne dopuszczalne wartoœci. Program obs³uguje wszystkie instrukcje
procesora, instrukcje 0xA5 traktuje jak nop. Obs³ugiwane s¹ wszystkie timery
i przerwania. 

Program wyposa¿ony jest w debugger pozwalaj¹cy kontrolowaæ dzia³anie
wgranego do pamiêci programu, wp³ywaæ na jego prace, modyfikowaæ pamiêæ
ustawiaæ breakpointy aktywowane na trzy ró¿ne sposoby: aktywowane ustawieniem
PC (program counter, lub te¿ IP - instruction pointer) na okreœlon¹ wartoœæ,
prób¹ odczytu okreœlonego adresu z wybranej pamiêci lub zmian¹ wartoœci
wybranych adresów w pamiêci.

INSTRUKCJA
========================

                               ...Podstawy...

Po uruchomieniu programu pojawia siê znak zachêty, który zazwyczaj wygl¹da tak:
cmd:>

Po nim, nale¿y podaæ jedn¹ z obs³ugiwanych przez program komend. Ich pe³n¹
listê mo¿na zobaczyæ wpisuj¹c komendê `help`.

Po uruchomieniu programu pamiêæ mikrokontrolera jest wyzerowana, mo¿na siê
o tym przekonaæ wpisuj¹c komende `hex rom` która spowoduje wyœwietlenie
zawartoœci pamiêci rom. 
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

Wyœwietlony listing jest bardzo d³ugi i zazwyczaj nie mieœci siê w oknie
konsoli. Mo¿na wiêc zapisaæ go do pliku zmianiaj¹c standardowe wyjœcie 
operatorem `>`.
cmd:>hex rom > pamiec_rom.txt

Po komendzie `hex` nale¿y podaæ nazwe pamiêci do wyœwietlenia, np. "rom", 
"intram", "extram", "sfr". Po nazwie mo¿na podaæ adres od którego ma zacz¹æ
byæ wyswietlana zawartoœæ pamiêci i adres koñcowy. Np:
cmd:>hex sfr 90 b0
0x0090 FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
0x00A0 FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
0x00B0 FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................

Aby za³adowaæ do pamiêci program nale¿y u¿yæ komendy `load` i podaæ lokalizacje
pliku intel hex z kodem programu. Np:
cmd:>load examples/hello.hex

Wykonanie tej komendy nie powoduje wyœwietlenia ¿¹dnego wyniku jeœli
program zosta³ wczytany do pamiêci bez problemu. Argumenty do komend
mo¿na podawaæ w cudzys³owach dziêki czemu mo¿na w nich zawieraæ spacje, inn¹
metod¹ wy³¹czenia traktowania bia³ych znaków, jako separatorów argumentów jest
poprzedzenie ich znakiem "\" (lewy ukoœnik). Analogicznie mo¿na poprzedziæ
cudzys³owy tym znakiem aby nie traktowaæ ich specjalne. Lewym ukoœnikiem
mo¿na te¿ poprzedziæ sam ukoœnik aby go dodaæ do argumentu. Przyk³ady:
cmd:>load "../rozne pliki/moj plik.hex"
cmd:>load ../rozne\ pliki/moj\ plik.hex
cmd:>load "..\\rozne pliki\\moj plik.hex"
cmd:>load ..\\rozne\ pliki\\moj\ plik.hex

Mo¿emy przekonaæ siê, ¿e program zosta³ za³adowany ponownie wykonuj¹c
komende `hex`:
cmd:>hex code 6c0 700
0x06C0 9B F5 83 22 20 F7 14 30 F6 14 88 83 A8 82 20 F5 ..." ..0...... .
0x06D0 07 E6 A8 83 75 83 00 22 E2 80 F7 E4 93 22 E0 22 ....u.."....."."
0x06E0 75 82 00 22 48 65 6C 6C 6F 20 77 6F 72 6C 64 21 u.."Hello world!
0x06F0 00 3C 4E 4F 20 46 4C 4F 41 54 3E 00 00 00 00 00 .<NO FLOAT>.....
0x0700 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................

Do kontroli dzia³ania wczytanego programu u¿ywamy przedewszystkim
komend `run`, `stop`, `output`, `hexoutput`, `step`, `trace`.

Na pocz¹tek omówimy komende `step`. Nie przyjmuje ona ¿adnych argumentów.
Jej dzia³anie jest bardzo proste. powoduje ona wykonanie jednej instrukcji
procesora, wypisanie na ekran informacji o wykonanej komendzie i niektóre
wartoœci. Adresy w pamiêciach IDATA, XDATA i SFR s¹ okreœlane zmieniane w
momêncie zmiany ich wartoœci.
cmd:>step
0008: 75 81 3A mov   SP, #3Ah
IDATA[00h]=00   XDATA[0000h]=00 SFR[SP]=3A      PSW=01          TCON=00
ACC=00          DPTR=0000       @DPTR=0000      SBUF[out]=00    SBUF[in]=00

U¿ywaj¹c komend `byte` i `bit` mo¿na wyœwietliæ, lub zmieniæ inne wybrane
komórki pamiêci:
cmd:>byte sfr p0
SFR[80]=FF
cmd:>bit sfr p0 1
SFR[80.1] = true
cmd:>bit sfr p0 1 0
cmd:>byte sfr p0
SFR[80]=FD

Kolejn¹ komend¹ pozwalaj¹c¹ wykonaæ kod jest `trace`. Komenda ta wyœwietla
wykonywane instrukcje. Komenda przyjmuje conajmniej jeden argument okreœlaj¹cy
iloœæ komend jakie maj¹ byc wykonane.
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

Podaj¹c dodatkowy parametr "1" funkcja wyœwietla wiêcej informacji. Z tym
parametrem jej dzia³anie przypomina wykonywanie kolejno instrukcji step.
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

W ka¿dej chwili mo¿na wyœwietliæ adres kolejnej instrukcji jaka zotanie
wykonana, czyli aktualny PC. Aby to zrobiæ nale¿y u¿yæ komendy `pc`.
cmd:>pc
PC = 003A

Podaj¹c argument do funkcji mo¿na zmieniæ aktualn¹ wartoœæ PC.

Komenda `run` uruchamia program w tle. W trakcie, gdy jest on uruchomiony
niektóre komendy moga nie dzia³aæ (zostanie wyœwietlony komunikat), 
jednak ca³y czas mo¿emy analizowaæ dzia³¹nie programu i zatrzymaæ go w 
dowolnym momencie komend¹ `stop`. Program, mo¿e sam zatrzymaæ wykonywanie
programu gdy mikrokontroler wyœle sygna³ PD (power down), natrafi na pêtle
z której nie mo¿e siê wydostaæ przyjednoczesnym braku zezwoleñ dla przerwañ lub
po ukatywnieniu jednego z breakpoint'ów.

W ka¿dej chwili mo¿na zresetowaæ mikrokontroler komedn¹ `reset` która przywraca
mikrokontroler do stanu pocz¹tkowego. Zeruje pamiêci z wyj¹tkiem pamiêci ROM.

Ostatnimi komendami s¹ output i hexoutput. Powoduj¹ one uruchomienie 
programu wraz z sukcesywnym wysy³aniem zawartoœci SBUF i pobieraniem
kodu klawiszy z klawiatury. Dzia³¹nie programu mo¿na zatrzymaæ w dowolnym
momêcie kombinacj¹ klawiszy Ctrl+Q (mo¿na zmieniæ komend¹ `setexitkey`).
cmd:>load hello.hex
cmd:>terminal
Hello world!

W ka¿dym momencie mo¿na wywo³aæ komende `state` która wyœwietla podstawowe
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

Ostatni¹ rzecz¹ o jakiej warto wspomnieæ w instrukcji jest disassembler.
Do jego wywo³ania s³u¿¹ funkcje `disassemble` i jej prostszy zamiennik `deasm`.
Komenda przyjmuje dwa argumenty, adres od którego wyœwietla kod i iloœæ
istrukcji do wyœwietlenia.
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

Skoro dotar³eœ tak daleko w czytaniu tego mam ma³¹ niespodziankê :-) !
Wykonaj tê komendê:
cmd:>exec for.fun

                            ...Breakpoint'y...

¯aden porz¹dny debugger nie obejdzie siê bez mo¿liwoœci stawiania breakpointów.
Aby utworzyæ breakpoint nalezy u¿yc komendy `breakpoint` lub jej krótszego
zamiennika `break`. Jej u¿ycie jest bardzo proste. Bo argumencie podajemy 
adresy w pamiêcy rom przy których dzia³anie programu ma zostaæ przerwane.
cmd:>break 041b
cmd:>run
cmd:>pc
PC = 041B
                           ...Pauzy warunkowe...

S51D obs³uguje dwa rodzaje pauz warunkowych, `conditionPause` i jej
prostszy zamiennik `cond`, i `accessPause` z zamiennikiem `access`.

`conditionPause` zatrzymuje dzia³anie kiedy wartoœæ w pamiêci ram lub
sfr spe³nia okreœlony warunek. Sk³adnia komendy wygl¹da tak:
conditionPause mem value operator addr ...
gdzie mem okreœla rodzaj pamiêci, value jest wartoœci¹ wzglêdem której
okreœlone addresy (addr) maj¹ byæ sprawdzone u¿ywaj¹c operatora.

Poni¿szy przyk³ad zatrzyma wykonywanie programu, kiedy pierwszy bit w komórce
pod adresem 0xFD w pamiêci zewnêtrznej RAM bêdzie w stanie wysokim:
cmd:>load sieve.hex
cmd:>cond xdata 1 and fd
cmd:>terminal
10 iterations

cmd:>hex xdata f0 ff
0x00F0 01 01 01 01 01 01 01 01 01 01 01 01 01 01 00 00 ................

`accessPause` zatrzymuje dzia³¹nie kiedy program spróbuje odczytwaæ, lub
zapisaæ coœ do okreœlonych komórek w pamiêci. W przypadku podania pamiêci
rom, program nie zatrzyma siê przy próbie odczytwania wartoœci przez
interpreter instrukcji, do tego s³u¿¹ breakpointy!
cmd:>load hello.hex
cmd:>access code 06ec
cmd:>terminal
Hello wo
cmd:>hex code 6e0 6ef
0x06E0 75 82 00 22 48 65 6C 6C 6F 20 77 6F 72 6C 64 21 u.."Hello world!
