# Schematic notes — verify before PCB

Signal connectivity follows `extra/docs/z280-s100-cpu-card.md`. These items need
verification against datasheets before this schematic is PCB-ready.

## Pin numbers to verify
- **Z280 (U1)**: 68-pin PLCC pinout verified — Z80 Family Data Book Fig. 2b
  (Z-BUS, OPT=1), transcribed from `extra/docs/z280-pins.tif`. Power = 2× VCC
  (18/19) + 4× GND (1/35/51/53); shared pins GREQ=CTIO0 (30), GACK=CTIN0 (32),
  EOP-A=INT-A (37), EOP-B=INT-B (36).
- **ATF1508 (U2/U24)**: pin numbers are now verified against the PLCC-84 pinout
  and match `z280-s100-control.pld` / `z280-s100-data.pld` — JTAG TDI/TMS/TCK/TDO
  at 14/23/62/71 (dedicated), VCCINT 3/43 + VCCIO 13/26/38/53/66/78, GND
  7/19/32/42/47/59/72/82, Z_CLK_IN on GCK1 = 83. Control uses 57 of 64 I/O, data
  63 of 64 (pin 84 = OE1 spare).
- **SST27SF020 flash (U11/U12)**: 32-pin DIP **socket**, JEDEC 27C020 layout
  (VPP=1, A16=2, A15=3, A17=30, PGM#=31, VDD=32; A0=12 … A14=29; DQ0=13 … DQ7=21;
  CE#=22, OE#=24, VSS=16). Word-addressed (flash A_n = byte A_{n+1}). The upper
  6 pins route through JP jumpers (J3–J8) so a smaller JEDEC EPROM can be fitted
  by moving the A15/A16/A17 jumpers; VPP/PGM#/VDD tie to +5V. A 28-pin part
  (27SF512, JEDEC +2 offset) puts its VDD on socket pin 30 — move J6 from A18 to
  +5V and leave J5/A16 unpopulated.

## Corrected S-100 pinout
Taken from `extra/hardware/s100z80/s100_Z80 V2-cache.lib` (S100_MALE). Key pins:
sMEMR=47, sWO=97, sINP=46, sOUT=45, sM1=44, sINTA=96, sHLTA=48, sXTRQ=58,
SIXTN=60, pSYNC=76, pDBIN=78, pWR=77, pSTVAL=25, pHLDA=26, pRDY=72, XRDY=3,
HOLD=74, RESET=75, INT=73, NMI=12, ADSB=22, DODSB=23, SDSB=18, CDSB=19,
DO0=36/DO1=35/DO2=88/DO3=89/DO4=38/DO5=39/DO6=40/DO7=90,
DI0=95/DI1=94/DI2=41/DI3=42/DI4=91/DI5=92/DI6=93/DI7=43.

## Wiring gaps (currently labeled but not fully connected)
- Interrupts: S100_INT / S100_NMI route through the CPLD to Z_INT / Z_NMI.
- Reset OR: DS1813 reset and S100_RESET must be OR'd before Z_RESET (diode-OR or
  CPLD input).
- LATCH_LE = ~AS (the CPLD inverts Z_AS to drive the 74HC573 latch enable).
- S-100 address drivers (A0-23, DO, status) need tri-state buffers (e.g. 74HCT244)
  gated by S100_A_OE / ADSB / DODSB / SDSB / CDSB — not yet placed.
- Power flags / ERC cleanup and decoupling caps not yet placed.
