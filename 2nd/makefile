# default the 32-bit arm compiler
PI_USER ?= evris
PI_HOST ?= anonymous.local
ARCH ?= 32

ifeq ($(ARCH), 64)
    # if the system is 64-bit
    CC = aarch64-linux-gnu-gcc
    SYSROOT = $(HOME)/rpi_sysroot_64
    ARCH_PATH = aarch64-linux-gnu
else
    # if the system is 32-bit
    CC = arm-linux-gnu-gcc
    SYSROOT = $(HOME)/rpi_sysroot
    ARCH_PATH = arm-linux-gnueabihf
endif

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

#sync the rpi with the system to make cross-complining
sync:
	@echo "=========================================================="
	@echo "Starting synchronization for Raspberry Pi..."
	@echo "User: $(PI_USER) | Host: $(PI_HOST) | Architecture: $(ARCH)-bit"
	@echo "To change these settings, run:"
	@echo "make sync PI_USER=<user> PI_HOST=<host> ARCH=<32/64>"
	@echo "=========================================================="
	mkdir -p $(SYSROOT)/usr $(SYSROOT)/lib
	rsync -avz --rsync-path="sudo rsync" $(PI_USER)@$(PI_HOST):/lib/ $(SYSROOT)/lib/
	rsync -avz --rsync-path="sudo rsync" $(PI_USER)@$(PI_HOST):/usr/include/ $(SYSROOT)/usr/include/
	rsync -avz --rsync-path="sudo rsync" $(PI_USER)@$(PI_HOST):/usr/lib/ $(SYSROOT)/usr/lib/
	
clean:
	rm -f $(OBJS) $(TARGET)