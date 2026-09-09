#include "spi_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>

// Default capacity for dynamic arrays
#define DEFAULT_CAPACITY 1000

spi_core_t* spi_core_init(const char* device_path, uint32_t baudrate) {
    spi_core_t* core = malloc(sizeof(spi_core_t));
    if (!core) {
        spi_core_log("Failed to allocate memory for SPI core");
        return NULL;
    }
    
    // Initialize core structure
    memset(core, 0, sizeof(spi_core_t));
    strncpy(core->device_path, device_path, sizeof(core->device_path) - 1);
    core->baudrate = baudrate;
    core->is_monitoring = false;
    
    // Initialize transaction array
    core->transaction_capacity = DEFAULT_CAPACITY;
    core->transactions = malloc(sizeof(spi_transaction_t) * core->transaction_capacity);
    if (!core->transactions) {
        spi_core_log("Failed to allocate memory for transactions");
        free(core);
        return NULL;
    }
    core->transaction_count = 0;
    
    // Initialize boot event array
    core->boot_event_capacity = DEFAULT_CAPACITY;
    core->boot_events = malloc(sizeof(boot_event_t) * core->boot_event_capacity);
    if (!core->boot_events) {
        spi_core_log("Failed to allocate memory for boot events");
        free(core->transactions);
        free(core);
        return NULL;
    }
    core->boot_event_count = 0;
    
    spi_core_log("SPI core initialized successfully");
    return core;
}

void spi_core_destroy(spi_core_t* core) {
    if (!core) return;
    
    // Free transactions
    if (core->transactions) {
        for (uint32_t i = 0; i < core->transaction_count; i++) {
            if (core->transactions[i].data) {
                free(core->transactions[i].data);
            }
            if (core->transactions[i].response) {
                free(core->transactions[i].response);
            }
        }
        free(core->transactions);
    }
    
    // Free boot events
    if (core->boot_events) {
        for (uint32_t i = 0; i < core->boot_event_count; i++) {
            if (core->boot_events[i].data) {
                free(core->boot_events[i].data);
            }
        }
        free(core->boot_events);
    }
    
    free(core);
    spi_core_log("SPI core destroyed");
}

bool spi_core_start_monitoring(spi_core_t* core) {
    if (!core) return false;
    
    if (core->is_monitoring) {
        spi_core_log("SPI monitoring already active");
        return false;
    }
    
    core->is_monitoring = true;
    spi_core_log("SPI monitoring started");
    return true;
}

void spi_core_stop_monitoring(spi_core_t* core) {
    if (!core) return;
    
    core->is_monitoring = false;
    spi_core_log("SPI monitoring stopped");
}

bool spi_core_add_transaction(spi_core_t* core, const spi_transaction_t* transaction) {
    if (!core || !transaction) return false;
    
    // Check if we need to expand the array
    if (core->transaction_count >= core->transaction_capacity) {
        uint32_t new_capacity = core->transaction_capacity * 2;
        spi_transaction_t* new_transactions = realloc(core->transactions, 
                                                     sizeof(spi_transaction_t) * new_capacity);
        if (!new_transactions) {
            spi_core_log("Failed to expand transaction array");
            return false;
        }
        core->transactions = new_transactions;
        core->transaction_capacity = new_capacity;
    }
    
    // Copy transaction data
    spi_transaction_t* new_transaction = &core->transactions[core->transaction_count];
    memcpy(new_transaction, transaction, sizeof(spi_transaction_t));
    
    // Copy data if present
    if (transaction->data && transaction->data_len > 0) {
        new_transaction->data = malloc(transaction->data_len);
        if (!new_transaction->data) {
            spi_core_log("Failed to allocate memory for transaction data");
            return false;
        }
        memcpy(new_transaction->data, transaction->data, transaction->data_len);
    }
    
    // Copy response if present
    if (transaction->response && transaction->response_len > 0) {
        new_transaction->response = malloc(transaction->response_len);
        if (!new_transaction->response) {
            spi_core_log("Failed to allocate memory for transaction response");
            if (new_transaction->data) free(new_transaction->data);
            return false;
        }
        memcpy(new_transaction->response, transaction->response, transaction->response_len);
    }
    
    core->transaction_count++;
    return true;
}

bool spi_core_add_boot_event(spi_core_t* core, const boot_event_t* event) {
    if (!core || !event) return false;
    
    // Check if we need to expand the array
    if (core->boot_event_count >= core->boot_event_capacity) {
        uint32_t new_capacity = core->boot_event_capacity * 2;
        boot_event_t* new_events = realloc(core->boot_events, 
                                          sizeof(boot_event_t) * new_capacity);
        if (!new_events) {
            spi_core_log("Failed to expand boot event array");
            return false;
        }
        core->boot_events = new_events;
        core->boot_event_capacity = new_capacity;
    }
    
    // Copy event data
    boot_event_t* new_event = &core->boot_events[core->boot_event_count];
    memcpy(new_event, event, sizeof(boot_event_t));
    
    // Copy data if present
    if (event->data && event->data_len > 0) {
        new_event->data = malloc(event->data_len);
        if (!new_event->data) {
            spi_core_log("Failed to allocate memory for boot event data");
            return false;
        }
        memcpy(new_event->data, event->data, event->data_len);
    }
    
    core->boot_event_count++;
    return true;
}

spi_transaction_t* spi_core_get_transactions(spi_core_t* core, uint32_t* count) {
    if (!core || !count) return NULL;
    
    *count = core->transaction_count;
    return core->transactions;
}

boot_event_t* spi_core_get_boot_events(spi_core_t* core, uint32_t* count) {
    if (!core || !count) return NULL;
    
    *count = core->boot_event_count;
    return core->boot_events;
}

bool spi_core_export_data(spi_core_t* core, const char* filename) {
    if (!core || !filename) return false;
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        spi_core_log("Failed to open file for export");
        return false;
    }
    
    fprintf(file, "{\n");
    fprintf(file, "  \"transactions\": [\n");
    
    for (uint32_t i = 0; i < core->transaction_count; i++) {
        spi_transaction_t* t = &core->transactions[i];
        fprintf(file, "    {\n");
        fprintf(file, "      \"timestamp\": %llu,\n", t->timestamp);
        fprintf(file, "      \"cs_pin\": %d,\n", t->cs_pin);
        fprintf(file, "      \"command\": 0x%02X,\n", t->command);
        fprintf(file, "      \"address\": 0x%08X,\n", t->address);
        fprintf(file, "      \"data_len\": %d,\n", t->data_len);
        fprintf(file, "      \"response_len\": %d,\n", t->response_len);
        fprintf(file, "      \"duration_us\": %u\n", t->duration_us);
        fprintf(file, "    }%s\n", (i < core->transaction_count - 1) ? "," : "");
    }
    
    fprintf(file, "  ],\n");
    fprintf(file, "  \"boot_events\": [\n");
    
    for (uint32_t i = 0; i < core->boot_event_count; i++) {
        boot_event_t* e = &core->boot_events[i];
        fprintf(file, "    {\n");
        fprintf(file, "      \"timestamp\": %llu,\n", e->timestamp);
        fprintf(file, "      \"event_type\": \"%s\",\n", e->event_type);
        fprintf(file, "      \"description\": \"%s\",\n", e->description);
        fprintf(file, "      \"data_len\": %d,\n", e->data_len);
        fprintf(file, "      \"significance\": %d\n", e->significance);
        fprintf(file, "    }%s\n", (i < core->boot_event_count - 1) ? "," : "");
    }
    
    fprintf(file, "  ]\n");
    fprintf(file, "}\n");
    
    fclose(file);
    spi_core_log("Data exported successfully");
    return true;
}

void spi_core_clear_data(spi_core_t* core) {
    if (!core) return;
    
    // Clear transactions
    for (uint32_t i = 0; i < core->transaction_count; i++) {
        if (core->transactions[i].data) {
            free(core->transactions[i].data);
        }
        if (core->transactions[i].response) {
            free(core->transactions[i].response);
        }
    }
    core->transaction_count = 0;
    
    // Clear boot events
    for (uint32_t i = 0; i < core->boot_event_count; i++) {
        if (core->boot_events[i].data) {
            free(core->boot_events[i].data);
        }
    }
    core->boot_event_count = 0;
    
    spi_core_log("Data cleared");
}

uint64_t get_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

void spi_core_log(const char* message) {
    printf("[SPI_CORE] %s\n", message);
}

bool spi_core_write_device(spi_core_t* core, const uint8_t* data, uint16_t len) {
    // Implementation would depend on the specific hardware interface
    // This is a placeholder for actual SPI communication
    (void)core;
    (void)data;
    (void)len;
    return true;
}

bool spi_core_read_device(spi_core_t* core, uint8_t* data, uint16_t len) {
    // Implementation would depend on the specific hardware interface
    // This is a placeholder for actual SPI communication
    (void)core;
    (void)data;
    (void)len;
    return true;
}
