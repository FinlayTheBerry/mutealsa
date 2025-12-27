CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror
DESTDIR ?=
PREFIX ?= /usr/lib

./sleepmute: ./sleepmute.c
	$(CC) $(CFLAGS) -s -o $@ $^ -lasound

debug: ./sleepmute_debug
./sleepmute_debug: ./sleepmute.c
	$(CC) $(CFLAGS) -g -O0 -o $@ $^ -lasound

static: ./sleepmute_static
./sleepmute_static: ./sleepmute.c | ./musl ./libalsa
	./musl/bin/gcc $(CFLAGS) -static -s -o $@ -I"./libalsa/build/include" $^ ./libalsa/build/lib/libasound.a

./libalsa: ./musl
	@echo "Cloning LibAlsa..."
	@git clone https://github.com/alsa-project/alsa-lib.git ./libalsa/ 1>/dev/null 2>&1
	@echo "Building LibAlsa..."
	@cd ./libalsa/ && autoreconf -i 1>/dev/null 2>&1
	@cd ./libalsa/ && CC="$$(realpath "../musl/bin/gcc")" ./configure --host=x86_64-linux-musl --prefix='' --disable-shared --enable-static --disable-python --disable-hwdep --disable-topology --disable-aload --disable-seq --disable-rawmidi --disable-ucm --disable-old-symbols --without-versioned --disable-dlset --with-pthread=yes 1>/dev/null 2>&1
	@cd ./libalsa/ && make -j$$(nproc) 1>/dev/null 2>&1
	@cd ./libalsa/ && make DESTDIR="$$(realpath "./build/")" install 1>/dev/null 2>&1
	@rm -rf ./libalsa/build/bin/ 1>/dev/null 2>&1
	@rm -rf ./libalsa/build/lib/pkgconfig/ 1>/dev/null 2>&1
	@rm -f ./libalsa/build/lib/libasound.la 1>/dev/null 2>&1
	@rm -rf ./libalsa/build/share/aclocal/ 1>/dev/null 2>&1

./musl:
	@echo "Downloading Musl..."
	@curl https://musl.cc/x86_64-linux-musl-native.tgz -o ./musl.tgz --progress-bar
	@echo "Extracting Musl..."
	@tar -xvf ./musl.tgz 1>/dev/null 2>&1
	@rm ./musl.tgz 1>/dev/null 2>&1
	@mv ./x86_64-linux-musl-native ./musl 1>/dev/null 2>&1

install:
	install -m755 -o0 -g0 ./sleepmute $(DESTDIR)/$(PREFIX)/systemd/system-sleep/sleepmute

install_static:
	install -m755 -o0 -g0 ./sleepmute_static $(DESTDIR)/$(PREFIX)/systemd/system-sleep/sleepmute

clean:
	rm -rf ./sleepmute ./sleepmute_debug ./sleepmute_static ./musl ./libalsa

.PHONY: debug static install install_static clean