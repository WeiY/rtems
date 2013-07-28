rtems_tests_PROGRAMS = smppflock
smppflock_SOURCES = init.c ../../support/src/locked_print.c

dist_rtems_tests_DATA = smppflock.scn smppflock.doc

include $(RTEMS_ROOT)/make/custom/@RTEMS_BSP@.cfg
include $(top_srcdir)/../automake/compile.am
include $(top_srcdir)/../automake/leaf.am

AM_CPPFLAGS += -I$(top_srcdir)/../support/include

LINK_OBJS = $(smppflock_OBJECTS)
LINK_LIBS = $(smppflock_LDLIBS)

smppflock$(EXEEXT): $(smppflock_OBJECTS) $(smppflock_DEPENDENCIES)
	@rm -f smppflock$(EXEEXT)
	$(make-exe)

include $(top_srcdir)/../automake/local.am
