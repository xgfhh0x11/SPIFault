# SPI Flash Manipulation Framework Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
LDFLAGS = -lm -lpthread

# Directories
SRC_DIR = src
EXAMPLES_DIR = examples
BUILD_DIR = build
DIST_DIR = dist

# Source files
SPI_CORE_SOURCES = $(SRC_DIR)/spi_core.c
INJECTION_SOURCES = $(SRC_DIR)/injection_engine.c
GLITCH_SOURCES = $(SRC_DIR)/glitch_engine.c
FRAMEWORK_SOURCES = $(SRC_DIR)/spi_framework.c

ALL_SOURCES = $(SPI_CORE_SOURCES) $(INJECTION_SOURCES) $(GLITCH_SOURCES) $(FRAMEWORK_SOURCES)

# Object files
SPI_CORE_OBJECTS = $(BUILD_DIR)/spi_core.o
INJECTION_OBJECTS = $(BUILD_DIR)/injection_engine.o
GLITCH_OBJECTS = $(BUILD_DIR)/glitch_engine.o
FRAMEWORK_OBJECTS = $(BUILD_DIR)/spi_framework.o

ALL_OBJECTS = $(SPI_CORE_OBJECTS) $(INJECTION_OBJECTS) $(GLITCH_OBJECTS) $(FRAMEWORK_OBJECTS)

# Libraries
SPI_LIB = $(BUILD_DIR)/libspi_core.a
INJECTION_LIB = $(BUILD_DIR)/libinjection.a
GLITCH_LIB = $(BUILD_DIR)/libglitch.a
FRAMEWORK_LIB = $(BUILD_DIR)/libspi_framework.a

# Executables
BASIC_EXAMPLE = $(BUILD_DIR)/basic_example

.PHONY: all clean install test examples

all: $(SPI_LIB) $(INJECTION_LIB) $(GLITCH_LIB) $(FRAMEWORK_LIB) examples

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# SPI Core Library
$(SPI_LIB): $(SPI_CORE_OBJECTS) | $(BUILD_DIR)
	ar rcs $@ $^

$(SPI_CORE_OBJECTS): $(SRC_DIR)/spi_core.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Injection Engine Library
$(INJECTION_LIB): $(INJECTION_OBJECTS) | $(BUILD_DIR)
	ar rcs $@ $^

$(INJECTION_OBJECTS): $(SRC_DIR)/injection_engine.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Glitch Engine Library
$(GLITCH_LIB): $(GLITCH_OBJECTS) | $(BUILD_DIR)
	ar rcs $@ $^

$(GLITCH_OBJECTS): $(SRC_DIR)/glitch_engine.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Framework Library
$(FRAMEWORK_LIB): $(FRAMEWORK_OBJECTS) | $(BUILD_DIR)
	ar rcs $@ $^

$(FRAMEWORK_OBJECTS): $(SRC_DIR)/spi_framework.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Examples
examples: $(BASIC_EXAMPLE)

$(BASIC_EXAMPLE): $(EXAMPLES_DIR)/basic_example.c $(FRAMEWORK_LIB) $(INJECTION_LIB) $(GLITCH_LIB) $(SPI_LIB) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $< -L$(BUILD_DIR) -lspi_framework -linjection -lglitch -lspi_core $(LDFLAGS) -o $@

# Test target
test: examples
	@echo "Running basic example..."
	@cd $(BUILD_DIR) && ./basic_example

# Install target
install: all
	@echo "Installing libraries..."
	@mkdir -p /usr/local/lib
	@mkdir -p /usr/local/include/spi_framework
	@cp $(BUILD_DIR)/*.a /usr/local/lib/
	@cp $(SRC_DIR)/*.h /usr/local/include/spi_framework/
	@echo "Installation complete"

# Clean target
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(DIST_DIR)

# Distribution target
dist: all
	@echo "Creating distribution package..."
	@mkdir -p $(DIST_DIR)/spi_framework
	@cp -r $(SRC_DIR) $(DIST_DIR)/spi_framework/
	@cp -r $(EXAMPLES_DIR) $(DIST_DIR)/spi_framework/
	@cp -r $(BUILD_DIR) $(DIST_DIR)/spi_framework/
	@cp Makefile $(DIST_DIR)/spi_framework/
	@cp README.md $(DIST_DIR)/spi_framework/
	@cd $(DIST_DIR) && tar -czf spi_framework.tar.gz spi_framework/
	@echo "Distribution package created: $(DIST_DIR)/spi_framework.tar.gz"

# Help target
help:
	@echo "Available targets:"
	@echo "  all      - Build all libraries and examples"
	@echo "  clean    - Remove build artifacts"
	@echo "  test     - Run basic example"
	@echo "  install  - Install libraries and headers"
	@echo "  dist     - Create distribution package"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Libraries:"
	@echo "  libspi_core.a     - SPI core functionality"
	@echo "  libinjection.a    - Injection engine"
	@echo "  libglitch.a       - Glitch engine"
	@echo "  libspi_framework.a - Main framework"
	@echo ""
	@echo "Examples:"
	@echo "  basic_example     - Basic usage example"