HOST_DIR ?= .
BUILDDIR ?= bin
MATERIALIZE ?= 0
ALGORITHM ?= SERIAL
NUM_THREAD ?= 4

HOST_SOURCES := $(wildcard ${HOST_DIR}/*.cpp)
HOST_TARGET := ${BUILDDIR}/host_code
HOST_FLAGS := -lpthread -lm -D${ALGORITHM} -DNUM_THREAD=${NUM_THREAD}

all : ${HOST_TARGET}

${HOST_TARGET} : ${HOST_SOURCES}
	g++ -o $@ ${HOST_SOURCES} ${HOST_FLAGS}

clean : ${HOST_TARGET}
	rm -rf $^