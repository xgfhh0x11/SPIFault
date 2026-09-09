#ifndef SPI_CORE_H
#define SPI_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// SPI Flash Commands
#define SPI_CMD_READ_ID          0x9F
#define SPI_CMD_READ_STATUS      0x05
#define SPI_CMD_WRITE_ENABLE     0x06
#define SPI_CMD_READ_DATA        0x03
#define SPI_CMD_PAGE_PROGRAM     0x02
#define SPI_CMD_SECTOR_ERASE     0x20
#define SPI_CMD_CHIP_ERASE       0xC7
#define SPI_CMD_POWER_DOWN       0xB9
#define SPI_CMD_RELEASE_POWER_DOWN 0xAB

// SPI Transaction Structure
typedef struct {
    uint64_t timestamp;
    uint8_t cs_pin;
    uint8_t command;
    uint32_t address;
    uint8_t *data;
    uint16_t data_len;
    uint8_t *response;
    uint16_t response_len;
    uint32_t duration_us;
} spi_transaction_t;

// Boot Event Structure
typedef struct {
    uint64_t timestamp;
    char event_type[32];
    char description[128];
    uint8_t *data;
    uint16_t data_len;
    uint8_t significance;
} boot_event_t;

// SPI Core Structure
typedef struct {
    char device_path[256];
    uint32_t baudrate;
    bool is_monitoring;
    spi_transaction_t *transactions;
    uint32_t transaction_count;
    uint32_t transaction_capacity;
    boot_event_t *boot_events;
    uint32_t boot_event_count;
    uint32_t boot_event_capacity;
} spi_core_t;

// Function Prototypes
spi_core_t* spi_core_init(const char* device_path, uint32_t baudrate);
void spi_core_destroy(spi_core_t* core);
bool spi_core_start_monitoring(spi_core_t* core);
void spi_core_stop_monitoring(spi_core_t* core);
bool spi_core_add_transaction(spi_core_t* core, const spi_transaction_t* transaction);
bool spi_core_add_boot_event(spi_core_t* core, const boot_event_t* event);
spi_transaction_t* spi_core_get_transactions(spi_core_t* core, uint32_t* count);
boot_event_t* spi_core_get_boot_events(spi_core_t* core, uint32_t* count);
bool spi_core_export_data(spi_core_t* core, const char* filename);
void spi_core_clear_data(spi_core_t* core);

// Utility Functions
uint64_t get_timestamp_us(void);
void spi_core_log(const char* message);
bool spi_core_write_device(spi_core_t* core, const uint8_t* data, uint16_t len);
bool spi_core_read_device(spi_core_t* core, uint8_t* data, uint16_t len);

#endif // SPI_CORE_H
