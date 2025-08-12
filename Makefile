CC = clang
CFLAGS = -Wall -O2 -Isrc
FRAMEWORKS = -framework Carbon -framework CoreFoundation -framework ApplicationServices
SRCDIR = src
BUILDDIR = build
TARGET = $(BUILDDIR)/launcher

# source files
SOURCES = $(SRCDIR)/launcher.c \
          $(SRCDIR)/dock_reader.c \
          $(SRCDIR)/hotkey_manager.c \
          $(SRCDIR)/app_controller.c \
          $(SRCDIR)/usage_tracker.c

OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

all: $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(FRAMEWORKS) -o $(TARGET)

clean:
	rm -rf $(BUILDDIR)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run