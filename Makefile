MODULE_big = jsonb_plpython
OBJS = jsonb_plpython.o $(WIN32RES)
PGFILEDESC = "jsonb_plpython - transform between jsonb and plpythonu"

EXTENSION = jsonb_plpython
# DATA = jsonb_plpython--1.0.sql jsonb_plpython2u--1.0.sql jsonb_plpython3u--1.0.sql
DATA = jsonb_plpython2u--1.0.sql

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
override CPPFLAGS := -I. -I${top_srcdir}/src/pl/plpython -I/home/anthony/Documents/work/postgresql/src/include/utils/ -I/home/anthony/Documents/work/postgresql/contrib/hstore/ -I/home/anthony/Documents/work/postgresql/src/pl/plpython/ $(python_includespec) $(CPPFLAGS) -DPLPYTHON_LIBNAME='"plpython$(python_majorversion)"'
else
subdir = contrib/jsonb_plpython
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

# We must link libpython explicitly
ifeq ($(PORTNAME), win32)
# ... see silliness in plpython Makefile ...
SHLIB_LINK += $(sort $(wildcard ../../src/pl/plpython/libpython*.a))
else
rpathdir = $(python_libdir)
SHLIB_LINK += $(python_libspec) $(python_additional_libs)
endif

#include $(top_srcdir)/src/pl/plpython/regress-python3-mangle.mk
