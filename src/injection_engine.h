#ifndef INJECTION_ENGINE_H
#define INJECTION_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

// Injection modes — how a payload is combined with the original data stream.
typedef enum {
    INJECTION_MODE_PASSIVE = 0,   // observe only, do not modify
    INJECTION_MODE_ACTIVE,        // append payload after original
    INJECTION_MODE_REPLACE,       // replace original with payload
    INJECTION_MODE_INSERT,        // insert payload at target offset
    INJECTION_MODE_MODIFY         // overwrite bytes at target offset
} injection_mode_t;

// Injection triggers — events that gate a rule.
typedef enum {
    INJECTION_TRIGGER_ON_COMMAND = 0,
    INJECTION_TRIGGER_ON_ADDRESS,
    INJECTION_TRIGGER_ON_DATA_PATTERN,
    INJECTION_TRIGGER_ON_TIMING,
    INJECTION_TRIGGER_MANUAL
} injection_trigger_t;

// Payload taxonomy — describes the *role* of the payload bytes within a
// fault-injection campaign. These categories are descriptive: they describe
// what the campaign's payload represents, not how it is to be weaponised.
typedef enum {
    PAYLOAD_TYPE_RAW = 0,             // arbitrary byte sequence, no framing
    PAYLOAD_TYPE_FIRMWARE_PATCH,      // bytes meant to overwrite firmware regions
    PAYLOAD_TYPE_BOOT_IMAGE,          // bytes interpreted as a boot-stage image
    PAYLOAD_TYPE_FAULT_PATTERN,       // structured test pattern (e.g. AA/55/00/FF)
    PAYLOAD_TYPE_RECOVERY_VECTOR      // recovery / fault-tolerance test data
} payload_type_t;

// Injection timing model — when, relative to the trigger, the payload is
// applied. Replaces the older exploit-flavored "technique" enum.
typedef enum {
    INJECTION_TIMING_ALIGNED = 0,     // synchronous to trigger event
    INJECTION_TIMING_DEFERRED,        // after a fixed delay
    INJECTION_TIMING_RANDOM,          // random delay within a window
    INJECTION_TIMING_BURST,           // repeated short pulses
    INJECTION_TIMING_TRIGGERED,       // gated on a secondary trigger
    INJECTION_TIMING_PERIODIC         // fixed-period repetition
} injection_timing_t;

// Payload structure.
typedef struct {
    char name[64];
    payload_type_t type;
    uint8_t *data;
    uint32_t size;
    uint32_t target_address;
    injection_timing_t timing;
    uint8_t noise_level;              // 0-10: how much background traffic to overlay
    char dependencies[8][32];
    uint8_t dependency_count;
} payload_t;

// Injection rule structure.
typedef struct {
    char name[64];
    injection_trigger_t trigger;
    union {
        uint8_t command;
        uint32_t address;
        char pattern[32];
    } trigger_value;
    injection_mode_t mode;
    payload_t *payload;
    uint32_t target_address;
    uint32_t timing_delay_us;
    bool enabled;
} injection_rule_t;

// Injection result structure.
typedef struct {
    uint64_t timestamp;
    char rule_name[64];
    uint8_t *original_data;
    uint16_t original_len;
    uint8_t *injected_data;
    uint16_t injected_len;
    bool success;
    char error_message[128];
} injection_result_t;

// Injection engine structure.
typedef struct {
    injection_rule_t *rules;
    uint32_t rule_count;
    uint32_t rule_capacity;
    injection_result_t *results;
    uint32_t result_count;
    uint32_t result_capacity;
    payload_t *payloads;
    uint32_t payload_count;
    uint32_t payload_capacity;
    bool is_active;
} injection_engine_t;

// Function prototypes.
injection_engine_t* injection_engine_init(void);
void injection_engine_destroy(injection_engine_t* engine);
bool injection_engine_add_rule(injection_engine_t* engine, const injection_rule_t* rule);
bool injection_engine_remove_rule(injection_engine_t* engine, const char* rule_name);
bool injection_engine_enable_rule(injection_engine_t* engine, const char* rule_name);
bool injection_engine_disable_rule(injection_engine_t* engine, const char* rule_name);
bool injection_engine_start_injection(injection_engine_t* engine);
void injection_engine_stop_injection(injection_engine_t* engine);
bool injection_engine_inject_manual(injection_engine_t* engine, const char* rule_name,
                                   const uint8_t* data, uint16_t len);
bool injection_engine_register_payload(injection_engine_t* engine, const payload_t* payload);
payload_t* injection_engine_get_payload(injection_engine_t* engine, const char* name);
injection_result_t* injection_engine_get_results(injection_engine_t* engine, uint32_t* count);
void injection_engine_clear_results(injection_engine_t* engine);
bool injection_engine_export_results(injection_engine_t* engine, const char* filename);

// Payload construction helpers.
payload_t* create_raw_payload(const char* name, const uint8_t* bytes,
                              uint32_t length, uint32_t target_address);
payload_t* create_firmware_patch(const char* name, const uint8_t* original_data,
                                uint32_t original_len, const uint8_t* patched_data,
                                uint32_t patched_len, uint32_t target_address);
payload_t* create_boot_image_payload(const char* name, const uint8_t* image_data,
                                     uint32_t image_len);
payload_t* create_fault_pattern_payload(const char* name, uint8_t pattern_byte,
                                        uint32_t length, uint32_t target_address);

// Utility functions.
void injection_engine_log(const char* message);
bool injection_engine_process_data(injection_engine_t* engine, const uint8_t* data, uint16_t len);
bool injection_engine_apply_injection(const injection_rule_t* rule, const uint8_t* original_data,
                                     uint16_t original_len, uint8_t** injected_data,
                                     uint16_t* injected_len);

#endif // INJECTION_ENGINE_H
