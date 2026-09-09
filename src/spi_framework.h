#ifndef SPI_FRAMEWORK_H
#define SPI_FRAMEWORK_H

#include "spi_core.h"
#include "injection_engine.h"
#include "glitch_engine.h"

// Main Framework Structure
typedef struct {
    spi_core_t* spi_core;
    injection_engine_t* injection_engine;
    glitch_engine_t* glitch_engine;
    bool is_active;
} spi_framework_t;

// Function Prototypes
spi_framework_t* spi_framework_init(const char* device_path, uint32_t baudrate);
void spi_framework_destroy(spi_framework_t* framework);
bool spi_framework_start(spi_framework_t* framework);
void spi_framework_stop(spi_framework_t* framework);
bool spi_framework_is_active(spi_framework_t* framework);

// Utility Functions
void spi_framework_log(const char* message);
bool spi_framework_export_all_data(spi_framework_t* framework, const char* base_filename);

#endif // SPI_FRAMEWORK_H
