CC     := clang
CFLAGS := $(shell pkg-config --static --cflags libavformat libavcodec glfw3)
CFLAGS += -Ofast -march=native -mtune=native -flto
CFLAGS += -ffast-math -funroll-loops -fomit-frame-pointer
CFLAGS += -DGL_SILENCE_DEPRECATION
SRCDIR   := src
INCDIR   := include
BUILDDIR := build
BINDIR   := bin
CFLAGS += -I$(INCDIR)
LDFLAGS := $(shell pkg-config --static --libs libavformat libavcodec glfw3)
LDFLAGS += -framework OpenGL -flto
SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))
EXEC := $(BINDIR)/main

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
