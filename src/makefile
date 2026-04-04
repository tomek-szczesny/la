
# Compiler and flags
CC = gcc
CFLAGS = -O3 -Wall -Wextra -std=c99 -g -I/usr/include/cairo -I/usr/include/libxml2
LDFLAGS = -lm

# Directories
OBJDIR = .build
DEPDIR = .deps
BINDIR = bin

# Installation directory
PREFIX = /usr/local
BINPREFIX = $(PREFIX)/bin

# Helper function to add object directory prefix
obj = $(addprefix $(OBJDIR)/, $(1:=.o))

# Source files
COMMON_SRCS = chains.c line.c plot.c gcode.c svg_path.c svg_transform.c
COMMON_OBJS = $(addprefix $(OBJDIR)/, $(COMMON_SRCS:.c=.o))

# Executables (files starting with la_)
EXECS = la_bounds la_dedup la_dup la_g2pdf la_g2svg la_opt la_pos la_rot la_strip la_svg2g
EXEC_TARGETS = $(addprefix $(BINDIR)/, $(EXECS))

# Dependency files
DEPS = $(wildcard $(DEPDIR)/*.d)

# Phony targets
.PHONY: all clean install uninstall

# Default target
all: $(EXEC_TARGETS)

# Create directories
$(shell mkdir -p $(OBJDIR) $(DEPDIR) $(BINDIR))

# Compile object files
$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@) $(dir $(DEPDIR)/$*.d)
	$(CC) $(CFLAGS) -MMD -MP -MF $(DEPDIR)/$*.d -c $< -o $@

# Link executables
$(BINDIR)/la_bounds: $(call obj, la_bounds gcode line)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(BINDIR)/la_dedup: $(call obj, la_dedup gcode line)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(BINDIR)/la_dup: $(call obj, la_dup gcode line)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(BINDIR)/la_g2pdf: $(call obj, la_g2pdf gcode line plot)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -lcairo -o $@

$(BINDIR)/la_g2svg: $(call obj, la_g2svg gcode line plot)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -lcairo -o $@

$(BINDIR)/la_opt: $(call obj, la_opt gcode line chains)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(BINDIR)/la_pos: $(call obj, la_pos gcode line)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(BINDIR)/la_rot: $(call obj, la_rot gcode line)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(BINDIR)/la_strip: $(call obj, la_strip gcode line)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(BINDIR)/la_svg2g: $(call obj, la_svg2g gcode line svg_transform svg_path)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -lxml2 -o $@

# Include dependency files
-include $(DEPS)

# Installation
install: $(EXEC_TARGETS)
	@mkdir -p $(BINPREFIX)
	@for exec in $(EXEC_TARGETS); do \
		install -m 755 $$exec $(BINPREFIX)/$$(basename $$exec); \
		echo "Installed $$(basename $$exec)"; \
	done

# Uninstallation
uninstall:
	@for exec in $(EXECS); do \
		rm -f $(BINPREFIX)/$$exec; \
		echo "Removed $$exec"; \
	done

# Clean build artifacts
clean:
	rm -rf $(OBJDIR) $(DEPDIR) $(BINDIR)
	@echo "Clean complete"

