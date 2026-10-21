#
# the ESP32 network card firmware - a host side build, so GNUmakefile
#
# micronix/stand/nic/GNUmakefile
#
# The card is an ESP32-DOIT DEVKIT V1; PlatformIO owns the tool chain.
# It has no place in the tree's cross build (this directory is not in
# stand's DIRS), so this file is only a conventional front end:
#
#	make		build the firmware
#	make upload	flash it over its USB-UART
#	make monitor	the serial console
#	make clean
#
# wifi credentials go in secrets.h (ignored by git) or in the tree's
# build flags -DWIFI_SSID=... -DWIFI_PASS=...; secrets.example.h is the
# template.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

PIO	?= pio

.PHONY: all upload flash monitor clean erase

all:
	$(PIO) run

upload flash:
	$(PIO) run -t upload

monitor:
	$(PIO) device monitor

clean:
	$(PIO) run -t clean

erase:
	$(PIO) run -t erase
