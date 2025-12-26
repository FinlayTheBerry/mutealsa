CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror -Wpedantic
DESTDIR ?=
PREFIX ?= /usr/lib

./mutealsa: ./mutealsa.c
	$(CC) $(CFLAGS) -s -o $@ $^ -lasound

debug: ./mutealsa_debug
./mutealsa_debug: ./mutealsa.c
	$(CC) $(CFLAGS) -g -O0 -o $@ $^ -lasound

install:
	install -m755 -o0 -g0 ./mutealsa $(DESTDIR)/$(PREFIX)/systemd/system-sleep/mutealsa

nixos_install:
	@install -m755 -o0 -g0 ./mutealsa /etc/nixos/bin/mutealsa
	@echo 'Please add the following to your nix config...'
	@echo 'powerManagement = {'
	@echo '    enable = true;'
	@echo '    powerDownCommands = "/etc/nixos/bin/mutealsa pre";'
	@echo '    powerUpCommands = "/etc/nixos/bin/mutealsa post";'
	@echo '};'

clean:
	rm -f ./mutealsa ./mutealsa_debug

.PHONY: debug install nixos_install clean