HOST_DIR ?= .
BUILDDIR ?= bin
MATERIALIZE ?= 0
ALGORITHM ?= SERIAL
NUM_THREAD ?= 4

HOST_SOURCES := ./gaussian.cpp
HOST_TARGET := ${BUILDDIR}/host_code
HOST_FLAGS := -lpthread -lm -D${ALGORITHM} -DNUM_THREAD=${NUM_THREAD}

all : ${HOST_TARGET}

${HOST_TARGET} : ${HOST_SOURCES}
	g++ -o $@ ${HOST_SOURCES} ${HOST_FLAGS}

clean :
	rm -rf ${HOST_TARGET}

SPECIAL_SOURCES := ./openmp_gaussian.cpp

omp : ${SPECIAL_SOURCES}
	g++ -o ${HOST_TARGET} ${SPECIAL_SOURCES} ${HOST_FLAGS} -fopenmp