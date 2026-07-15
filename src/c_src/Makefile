# QNX recursive makefile: OS level
#LIST=OS
#include recurse.mki

TARGET=camera-dump-frame-no-screen
SRC=camera_dump_frame_no_screen.c

LIBS=lcamapi

all: $(TARGET)

$(TARGET):
	qcc -V gcc_ntoaarch64le $(SRC) -o $(TARGET) -$(LIBS)

DEBUG:
	qcc -V gcc_ntoaarch64le$(SRC) -o $(TARGET)-dbg -$(LIBS)

clean:
	rm -f *.o $(TARGET)
