bin_PROGRAMS  = grist 

if COND_MEMWATCH
grist_SOURCES =	main.c \
		config.c \
		db_sql.c \
		../memwatch/memwatch.c

noinst_HEADERS = grist.h \
		 db_sql.h \
		 ../memwatch/memwatch.h

INCLUDES=-I../memwatch
else
grist_SOURCES =	main.c \
		config.c \
		db_sql.c 

noinst_HEADERS = grist.h \
		 db_sql.h 

INCLUDES=
endif

