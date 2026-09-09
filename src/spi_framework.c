#include "spi_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

spi_framework_t* spi_framework_init(const char* device_path, uint32_t baudrate) {
    spi_framework_t* framework = malloc(sizeof(spi_framework_t));
    if (!framework) {
        spi_framework_log("Failed to allocate memory for SPI framework");
        return NULL;
    }
    
    memset(framework, 0, sizeof(spi_framework_t));
    
    // Initialize SPI core
    framework->spi_core = spi_core_init(device_path, baudrate);
    if (!framework->spi_core) {
        spi_framework_log("Failed to initialize SPI core");
        free(framework);
        return NULL;
    }
    
    // Initialize injection engine
    framework->injection_engine = injection_engine_init();
    if (!framework->injection_engine) {
        spi_framework_log("Failed to initialize injection engine");
        spi_core_destroy(framework->spi_core);
        free(framework);
        return NULL;
    }
    
    // Initialize glitch engine
    framework->glitch_engine = glitch_engine_init(device_path);
    if (!framework->glitch_engine) {
        spi_framework_log("Failed to initialize glitch engine");
        spi_core_destroy(framework->spi_core);
        injection_engine_destroy(framework->injection_engine);
        free(framework);
        return NULL;
    }
    
    framework->is_active = false;
    spi_framework_log("SPI framework initialized successfully");
    return framework;
}

void spi_framework_destroy(spi_framework_t* framework) {
    if (!framework) return;
    
    // Stop framework if active
    if (framework->is_active) {
        spi_framework_stop(framework);
    }
    
    // Destroy all components
    if (framework->spi_core) {
        spi_core_destroy(framework->spi_core);
    }
    
    if (framework->injection_engine) {
        injection_engine_destroy(framework->injection_engine);
    }
    
    if (framework->glitch_engine) {
        glitch_engine_destroy(framework->glitch_engine);
    }
    
    free(framework);
    spi_framework_log("SPI framework destroyed");
}

bool spi_framework_start(spi_framework_t* framework) {
    if (!framework) return false;
    
    if (framework->is_active) {
        spi_framework_log("Framework already active");
        return false;
    }
    
    // Start SPI monitoring
    if (!spi_core_start_monitoring(framework->spi_core)) {
        spi_framework_log("Failed to start SPI monitoring");
        return false;
    }
    
    // Start injection engine
    if (!injection_engine_start_injection(framework->injection_engine)) {
        spi_framework_log("Failed to start injection engine");
        spi_core_stop_monitoring(framework->spi_core);
        return false;
    }
    
    // Start glitch engine
    if (!glitch_engine_start_glitching(framework->glitch_engine)) {
        spi_framework_log("Failed to start glitch engine");
        spi_core_stop_monitoring(framework->spi_core);
        injection_engine_stop_injection(framework->injection_engine);
        return false;
    }
    
    framework->is_active = true;
    spi_framework_log("SPI framework started successfully");
    return true;
}

void spi_framework_stop(spi_framework_t* framework) {
    if (!framework) return;
    
    if (!framework->is_active) {
        spi_framework_log("Framework not active");
        return;
    }
    
    // Stop all components
    spi_core_stop_monitoring(framework->spi_core);
    injection_engine_stop_injection(framework->injection_engine);
    glitch_engine_stop_glitching(framework->glitch_engine);
    
    framework->is_active = false;
    spi_framework_log("SPI framework stopped");
}

// NOTE: not called from any path in this repository (public API only —
// no caller in src/ or examples/).
bool spi_framework_is_active(spi_framework_t* framework) {
    if (!framework) return false;
    return framework->is_active;
}

void spi_framework_log(const char* message) {
    printf("[FRAMEWORK] %s\n", message);
}

bool spi_framework_export_all_data(spi_framework_t* framework, const char* base_filename) {
    if (!framework || !base_filename) return false;
    
    char filename[256];
    bool success = true;
    
    // Export SPI data
    snprintf(filename, sizeof(filename), "%s_spi.json", base_filename);
    if (!spi_core_export_data(framework->spi_core, filename)) {
        spi_framework_log("Failed to export SPI data");
        success = false;
    }
    
    // Export injection results
    snprintf(filename, sizeof(filename), "%s_injection.json", base_filename);
    if (!injection_engine_export_results(framework->injection_engine, filename)) {
        spi_framework_log("Failed to export injection results");
        success = false;
    }
    
    // Export glitch results
    snprintf(filename, sizeof(filename), "%s_glitch.json", base_filename);
    if (!glitch_engine_export_results(framework->glitch_engine, filename)) {
        spi_framework_log("Failed to export glitch results");
        success = false;
    }
    
    if (success) {
        spi_framework_log("All data exported successfully");
    }
    
    return success;
}
