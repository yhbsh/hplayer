CC       := clang
CFLAGS   := -I./include -Ofast -march=native -mtune=native -ffast-math -funroll-loops -fomit-frame-pointer -DGL_SILENCE_DEPRECATION
SRCDIR   := src
INCDIR   := include
BUILDDIR := build
BINDIR   := bin
LDFLAGS  := -lavcodec -lavformat -lavdevice -lswscale -lswresample -lavutil -lglfw3 -framework OpenGL -framework IOKit -framework Cocoa
SRCS     := $(wildcard $(SRCDIR)/*.c)
OBJS     := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))
EXEC     := $(BINDIR)/main

.PHONY: all clean directories

all: directories $(EXEC)

directories:
	@mkdir -p $(BUILDDIR) $(BINDIR)

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR) $(BINDIR)

-include $(OBJS:.o=.d)

$(BUILDDIR)/%.d: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MM -MT $(@:.d=.o) $< > $@
