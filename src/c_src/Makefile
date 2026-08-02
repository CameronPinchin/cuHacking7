##
## This is a custom makefile, purely for development.
##
CC = qcc
TARGET_FLAG = -Vgcc_ntoaarch64le

FFMPEG_ROOT = /home/cameron/qnx_workspace/ffmpeg/qnx_build/usr/local
IDIR = $(FFMPEG_ROOT)/include
LDIR = $(FFMPEG_ROOT)/lib

CFLAGS = $(TARGET_FLAG) -I$(IDIR) -O2 -Wall -g
LDFLAGS = -L$(LDIR)
LIBS = -lavformat -lavcodec -lswscale -lswresample -lavutil

TARGET = bin/drone_capture

all: $(TARGET)

$(TARGET): drone_operations.c
	$(CC) $(CFLAGS) drone_operations.c $(LDFLAGS) $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)
