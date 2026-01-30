CC         ?= gcc
PKG_CONFIG ?= pkg-config

SRCDIR := src
INCDIR := include
BINDIR := bin
BUILDDIR := build
TARGET := $(BINDIR)/fpga

SRCS := $(SRCDIR)/main.c $(SRCDIR)/program.c $(SRCDIR)/fpga.c $(SRCDIR)/gpio.c $(SRCDIR)/spi.c $(SRCDIR)/log_print.c
OBJS := $(SRCS:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

# Flags
CFLAGS  += -O2 -Wall -I$(INCDIR) $(shell $(PKG_CONFIG) --cflags libgpiod)
LDLIBS  += $(shell $(PKG_CONFIG) --libs   libgpiod)

# Default
all: $(TARGET)

# Link
$(TARGET): $(OBJS) | $(BINDIR)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

# Compile
$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Dirs
$(BINDIR) $(BUILDDIR):
	mkdir -p $@

# Housekeeping
.PHONY: clean
clean:
	rm -rf $(BUILDDIR) $(TARGET)
