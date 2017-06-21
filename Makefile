# contrib/pg_prewarm/Makefile

MODULE_big = jsonb_plpython
OBJS = jsonb_plpython.o $(WIN32RES)

EXTENSION = jsonb_plpython
DATA = jsonb_plpython.sql

PGFILEDESC = "jsonb_plpython - transform between jsonb and plpythonu"

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/jsonb_plpython
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
