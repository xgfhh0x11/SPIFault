#ifndef GLITCH_ENGINE_H
#define GLITCH_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

// Glitch Types
typedef enum {
    GLITCH_TYPE_VOLTAGE = 0,
    GLITCH_TYPE_CLOCK,
    GLITCH_TYPE_ELECTROMAGNETIC,
    GLITCH_TYPE_LASER_FAULT,
    GLITCH_TYPE_TEMPERATURE,
    GLITCH_TYPE_POWER_SUPPLY
} glitch_type_t;

// Glitch Timing
typedef enum {
    GLITCH_TIMING_RANDOM = 0,
    GLITCH_TIMING_SYNCHRONIZED,
    GLITCH_TIMING_TRIGGERED,
    GLITCH_TIMING_CONTINUOUS,
    GLITCH_TIMING_BURST
} glitch_timing_t;

// Fault Types
typedef enum {
    FAULT_TYPE_SINGLE_BIT_FLIP = 0,
    FAULT_TYPE_MULTI_BIT_FLIP,
    FAULT_TYPE_BYTE_CORRUPTION,
    FAULT_TYPE_INSTRUCTION_SKIP,
    FAULT_TYPE_BRANCH_OVERRIDE,
    FAULT_TYPE_MEMORY_CORRUPTION,
    FAULT_TYPE_REGISTER_CORRUPTION
} fault_type_t;

// Target Components
typedef enum {
    TARGET_COMPONENT_CPU_CORE = 0,
    TARGET_COMPONENT_MEMORY,
    TARGET_COMPONENT_CACHE,
    TARGET_COMPONENT_REGISTERS,
    TARGET_COMPONENT_BUS,
    TARGET_COMPONENT_CLOCK,
    TARGET_COMPONENT_POWER
} target_component_t;

// Glitch Parameters
typedef struct {
    glitch_type_t type;
    glitch_timing_t timing;
    float voltage_level;      // V
    uint32_t duration_ns;     // nanoseconds
    uint32_t frequency_hz;    // Hz
    uint32_t trigger_delay_ns; // nanoseconds
    uint32_t burst_count;
    uint32_t burst_interval_ns; // nanoseconds
} glitch_parameters_t;

// Fault Parameters
typedef struct {
    fault_type_t type;
    target_component_t target;
    uint32_t target_address;
    uint8_t bit_position;
    uint8_t *corruption_value;
    uint16_t corruption_len;
    uint32_t timing_offset_ns;
    uint32_t duration_ns;
    float intensity;          // 0.0 - 1.0
    uint32_t repeat_count;
    uint32_t repeat_interval_ns;
} fault_parameters_t;

// Glitch Result
typedef struct {
    uint64_t timestamp;
    glitch_parameters_t parameters;
    bool success;
    uint8_t *response_data;
    uint16_t response_len;
    char error_message[128];
    uint32_t execution_time_us;
} glitch_result_t;

// Fault Result
typedef struct {
    uint64_t timestamp;
    fault_parameters_t parameters;
    bool success;
    uint8_t *system_response;
    uint16_t response_len;
    char error_message[128];
} fault_result_t;

// Voltage Profile
typedef struct {
    char name[64];
    float base_voltage;       // V
    float glitch_voltage;     // V
    uint32_t glitch_duration_ns;
    uint32_t rise_time_ns;
    uint32_t fall_time_ns;
    uint32_t frequency_hz;
    float duty_cycle;         // 0.0 - 1.0
} voltage_profile_t;

// Glitch Engine Structure
typedef struct {
    glitch_parameters_t *glitch_params;
    uint32_t glitch_count;
    uint32_t glitch_capacity;
    glitch_result_t *glitch_results;
    uint32_t glitch_result_count;
    uint32_t glitch_result_capacity;
    fault_parameters_t *fault_params;
    uint32_t fault_count;
    uint32_t fault_capacity;
    fault_result_t *fault_results;
    uint32_t fault_result_count;
    uint32_t fault_result_capacity;
    voltage_profile_t *voltage_profiles;
    uint32_t voltage_profile_count;
    uint32_t voltage_profile_capacity;
    voltage_profile_t *active_profile;
    bool is_active;
    char device_path[256];
} glitch_engine_t;

// Function Prototypes
glitch_engine_t* glitch_engine_init(const char* device_path);
void glitch_engine_destroy(glitch_engine_t* engine);
bool glitch_engine_setup_hardware(glitch_engine_t* engine);
bool glitch_engine_add_glitch_parameters(glitch_engine_t* engine, const glitch_parameters_t* params);
bool glitch_engine_remove_glitch_parameters(glitch_engine_t* engine, uint32_t index);
bool glitch_engine_start_glitching(glitch_engine_t* engine);
void glitch_engine_stop_glitching(glitch_engine_t* engine);
bool glitch_engine_execute_glitch(glitch_engine_t* engine, const glitch_parameters_t* params);
bool glitch_engine_add_fault_parameters(glitch_engine_t* engine, const fault_parameters_t* params);
bool glitch_engine_remove_fault_parameters(glitch_engine_t* engine, uint32_t index);
bool glitch_engine_inject_fault(glitch_engine_t* engine, const fault_parameters_t* params);
bool glitch_engine_add_voltage_profile(glitch_engine_t* engine, const voltage_profile_t* profile);
bool glitch_engine_set_active_profile(glitch_engine_t* engine, const char* profile_name);
bool glitch_engine_single_glitch(glitch_engine_t* engine, uint32_t duration_ns);
bool glitch_engine_burst_glitch(glitch_engine_t* engine, uint32_t count, uint32_t duration_ns, uint32_t interval_ns);
glitch_result_t* glitch_engine_get_glitch_results(glitch_engine_t* engine, uint32_t* count);
fault_result_t* glitch_engine_get_fault_results(glitch_engine_t* engine, uint32_t* count);
void glitch_engine_clear_results(glitch_engine_t* engine);
bool glitch_engine_export_results(glitch_engine_t* engine, const char* filename);

// Utility Functions
void glitch_engine_log(const char* message);
voltage_profile_t* create_voltage_profile(const char* name, float base_voltage, 
                                         float glitch_voltage, uint32_t glitch_duration_ns);
fault_parameters_t* create_bypass_fault(uint32_t target_address, uint8_t bit_position);
fault_parameters_t* create_instruction_skip_fault(uint32_t target_address);
fault_parameters_t* create_branch_override_fault(uint32_t target_address);

#endif // GLITCH_ENGINE_H
