VERSION = 0.0.6

CC = gcc

CFLAGS = -Wall -Wextra -pedantic -Wno-deprecated-declarations -std=c99 ${CPPFLAGS}

DFLAGS = -g

PREFIX = /usr/local

DATAROOTDIR = ${PREFIX}/share

DATADIR = ${DATAROOTDIR}

MANPREFIX = ${DATADIR}/man

EXEC_PREFIX = ${PREFIX}

BINDIR = ${EXEC_PREFIX}/bin

all: options ti

options:
	@echo "ti build options:"
	@echo ""
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "CC       = ${CC}"

debug_options:
	@echo ""
	@echo "ti debugging build options:"
	@echo ""
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "DFLAGS   = ${DFLAGS}"
	@echo "CC       = ${CC}"
	@echo ""
	
help:
	@echo ""
	@echo "Ti ${VERSION}"
	@echo ""
	@echo "install - install binary and man page"
	@echo "uninstall - rm binary from install path"
	@echo "options - show current build options"
	@echo "debug - compile with debug info"
	@echo "clean - rm binary from current directory"
	@echo "dist - package into tarball"
	@echo ""

ti: ti.c
	${CC} $^ -o $@ ${CFLAGS}

debug: debug_options
	${CC} ${DFLAGS} ti.c -o tidebug ${CFLAGS}

clean:
	rm ti 
	if test -f "ti-${VERSION}.tar.gz"; then	\
		rm ti-${VERSION}.tar.gz; \
	fi

dist: clean
	mkdir -p ti-${VERSION}
	cp -R LICENSE Makefile README.md t.1 ti-${VERSION}
	tar -cf dwm-${VERSION}.tar dwm-${VERSION}
	gzip dwm-${VERSION}.tar
	rm -rf dwm-${VERSION}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ti ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/ti
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < ti.1 > ${DESTDIR}${MANPREFIX}/man1/ti.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/ti.1
	
uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/ti\
		${DESTDIR}${MANPREFIX}/man1/ti.1

.PHONY: all options clean dist install uninstall
