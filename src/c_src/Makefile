# QNX recursive makefile: OS level
#LIST=OS
#include recurse.mki

TARGET=camera-dump-frame-no-screen
DEV_TARGET=camera_operations
SRC=camera_dump_frame_no_screen.c
DEV_SRC=camera_operations.c

LIBS=lcamapi

all: $(TARGET) $(DEV_TARGET)

$(TARGET):
	qcc -V gcc_ntoaarch64le $(SRC) -o $(TARGET) -$(LIBS)

$(DEV_TARGET):
	qcc -V gcc_ntoaarch64le $(DEV_SRC) -o $(DEV_TARGET) -$(LIBS)

DEBUG:
	qcc -V gcc_ntoaarch64le$(SRC) -o $(TARGET)-dbg -$(LIBS)

clean:
	rm -f *.o $(TARGET) $(DEV_TARGET)
