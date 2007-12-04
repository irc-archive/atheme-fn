# Derived from contrib/Makefile.in
# $Id$
# Makefile.config should define source and prefix
include Makefile.config

CC		= gcc
RM		= /bin/rm
MV		= /bin/mv
CP		= /bin/cp
INSTALL		= /usr/bin/install -c
#source		= /home/jilles/src/svn/atheme
#prefix		= /home/jilles/ircd/atheme
exec_prefix	= ${prefix}
bindir		= ${exec_prefix}/bin
datadir		= ${prefix}/share
sysconfdir	= ${prefix}/etc
libdir		= ${exec_prefix}/lib
sbindir		= ${exec_prefix}/sbin
localstatedir	= ${prefix}/var
DOCDIR		= ${prefix}/doc
MODDIR		= ${exec_prefix}
SHAREDIR	= ${prefix}
MKDEP		= gcc -MM -DPREFIX=\"${prefix}\" -I${source}/include -I${source}/libmowgli/src/libmowgli
PICFLAGS	= -fPIC -DPIC -shared
CFLAGS		= -g -O2  -Wpointer-arith -Wimplicit -Wnested-externs -Wcast-align -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -W -Wno-unused -Wshadow -Wmissing-noreturn -Wundef -Wpacked -Wnested-externs -Wbad-function-cast -Wredundant-decls -Wfloat-equal -Wformat=2 -Wdisabled-optimization -DPREFIX=\"${prefix}\" -DMODDIR=\"${MODDIR}\" -DSHAREDIR=\"${prefix}\" -DSYSCONFDIR=\"${prefix}/etc\" -DLOGDIR=\"var\" -DRUNDIR=\"var\" -DDATADIR=\"etc\" -I${source}/include -I${source}/libmowgli/src/libmowgli

LIBS		=  -lcrypt -lssl -lcrypto
LDFLAGS		+=  -Wl,-export-dynamic
#CPPFLAGS	= 

default: all

SRCS = \
	cs_access.c		\
	ns_extendchans.c	\
	ns_regain.c	\
	ns_set_accountname.c	\
	ns_set_unfiltered.c	\
	os_greplog.c	\
	oftc-hybrid.c	\
	regnotice.c

# To compile your own modules, add them to SRCS or make blegh.so

OBJS = ${SRCS:.c=.so}

all: ${OBJS}

install:
	${INSTALL} -m 755 -d $(DESTDIR)${MODDIR}/modules/freenode
	${INSTALL} -m 755 *.so $(DESTDIR)${MODDIR}/modules/freenode
	$(INSTALL) -m 755 -d $(DESTDIR)$(SHAREDIR)/help
	(cd help; for i in *; do \
		[ -f $$i ] && $(INSTALL) -m 644 $$i $(DESTDIR)$(SHAREDIR)/help; \
		if [ -d $$i ]; then \
			cd $$i; \
			$(INSTALL) -m 755 -d $(DESTDIR)$(SHAREDIR)/help/$$i; \
			for j in *; do \
				[ -f $$j ] && $(INSTALL) -m 644 $$j $(DESTDIR)$(SHAREDIR)/help/$$i; \
			done; \
			cd ..; \
		fi; \
	done)

.SUFFIXES: .so

.c.so:
	${CC} ${PICFLAGS} ${CPPFLAGS} ${CFLAGS} $< -o $@

.PHONY: depend clean distclean
# This sed command sucks but I don't know a better way -- jilles
depend:
	${MKDEP} ${PICFLAGS} ${CPPFLAGS} ${CFLAGS} ${SRCS} | sed -e 's/\.o:/.so:/' > .depend

clean:
	${RM} -f *.so

distclean: clean
	${RM} -f Makefile version.c.last

# we don't really need this -- jilles
#include .depend
