# Fedora cross-compiler toolchain for ARM target architecture
CC = arm-linux-gnu-gcc

# Base directory for the local Raspberry Pi sysroot
SYSROOT = $(HOME)/rpi_sysroot

# Added arm-linux-gnueabihf paths to locate the correct target headers
CFLAGS = -Wall -Wextra -O2 --sysroot=$(SYSROOT) -I$(SYSROOT)/usr/include -I$(SYSROOT)/usr/include/arm-linux-gnueabihf

# Resolves secondary dynamic dependencies (e.g., zstd, atomic, cap) seamlessly
LDFLAGS = -L$(SYSROOT)/usr/lib/arm-linux-gnueabihf -L$(SYSROOT)/lib/arm-linux-gnueabihf -B$(SYSROOT)/usr/lib/arm-linux-gnueabihf -Wl,-rpath-link=$(SYSROOT)/usr/lib/arm-linux-gnueabihf -Wl,-rpath-link=$(SYSROOT)/lib/arm-linux-gnueabihf

# Core libraries required for the application
LIBS = -lpthread -lwebsockets -lssl -lcrypto -lm

# Project source files and corresponding object files
SRCS = main.c queue.c producer.c consumer.c monitor.c cJSON.c
OBJS = $(SRCS:.c=.o)

# Final executable output name
TARGET = firehose_app

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.c my_pthreads.h cJSON.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)