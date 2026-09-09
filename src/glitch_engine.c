#include "glitch_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_CAPACITY 100

glitch_engine_t* glitch_engine_init(const char* device_path) {
    glitch_engine_t* engine = malloc(sizeof(glitch_engine_t));
    if (!engine) {
        glitch_engine_log("Failed to allocate memory for glitch engine");
        return NULL;
    }
    
    memset(engine, 0, sizeof(glitch_engine_t));
    
    if (device_path) {
        strncpy(engine->device_path, device_path, sizeof(engine->device_path) - 1);
    }
    
    // Initialize glitch parameters array
    engine->glitch_capacity = DEFAULT_CAPACITY;
    engine->glitch_params = malloc(sizeof(glitch_parameters_t) * engine->glitch_capacity);
    if (!engine->glitch_params) {
        glitch_engine_log("Failed to allocate memory for glitch parameters");
        free(engine);
        return NULL;
    }
    engine->glitch_count = 0;
    
    // Initialize glitch results array
    engine->glitch_result_capacity = DEFAULT_CAPACITY;
    engine->glitch_results = malloc(sizeof(glitch_result_t) * engine->glitch_result_capacity);
    if (!engine->glitch_results) {
        glitch_engine_log("Failed to allocate memory for glitch results");
        free(engine->glitch_params);
        free(engine);
        return NULL;
    }
    engine->glitch_result_count = 0;
    
    // Initialize fault parameters array
    engine->fault_capacity = DEFAULT_CAPACITY;
    engine->fault_params = malloc(sizeof(fault_parameters_t) * engine->fault_capacity);
    if (!engine->fault_params) {
        glitch_engine_log("Failed to allocate memory for fault parameters");
        free(engine->glitch_params);
        free(engine->glitch_results);
        free(engine);
        return NULL;
    }
    engine->fault_count = 0;
    
    // Initialize fault results array
    engine->fault_result_capacity = DEFAULT_CAPACITY;
    engine->fault_results = malloc(sizeof(fault_result_t) * engine->fault_result_capacity);
    if (!engine->fault_results) {
        glitch_engine_log("Failed to allocate memory for fault results");
        free(engine->glitch_params);
        free(engine->glitch_results);
        free(engine->fault_params);
        free(engine);
        return NULL;
    }
    engine->fault_result_count = 0;
    
    // Initialize voltage profiles array
    engine->voltage_profile_capacity = DEFAULT_CAPACITY;
    engine->voltage_profiles = malloc(sizeof(voltage_profile_t) * engine->voltage_profile_capacity);
    if (!engine->voltage_profiles) {
        glitch_engine_log("Failed to allocate memory for voltage profiles");
        free(engine->glitch_params);
        free(engine->glitch_results);
        free(engine->fault_params);
        free(engine->fault_results);
        free(engine);
        return NULL;
    }
    engine->voltage_profile_count = 0;
    engine->active_profile = NULL;
    
    engine->is_active = false;
    glitch_engine_log("Glitch engine initialized");
    return engine;
}

void glitch_engine_destroy(glitch_engine_t* engine) {
    if (!engine) return;
    
    // Free glitch parameters
    if (engine->glitch_params) {
        free(engine->glitch_params);
    }
    
    // Free glitch results
    if (engine->glitch_results) {
        for (uint32_t i = 0; i < engine->glitch_result_count; i++) {
            if (engine->glitch_results[i].response_data) {
                free(engine->glitch_results[i].response_data);
            }
        }
        free(engine->glitch_results);
    }
    
    // Free fault parameters
    if (engine->fault_params) {
        for (uint32_t i = 0; i < engine->fault_count; i++) {
            if (engine->fault_params[i].corruption_value) {
                free(engine->fault_params[i].corruption_value);
            }
        }
        free(engine->fault_params);
    }
    
    // Free fault results
    if (engine->fault_results) {
        for (uint32_t i = 0; i < engine->fault_result_count; i++) {
            if (engine->fault_results[i].system_response) {
                free(engine->fault_results[i].system_response);
            }
        }
        free(engine->fault_results);
    }
    
    // Free voltage profiles
    if (engine->voltage_profiles) {
        free(engine->voltage_profiles);
    }
    
    free(engine);
    glitch_engine_log("Glitch engine destroyed");
}

bool glitch_engine_setup_hardware(glitch_engine_t* engine) {
    if (!engine) return false;
    
    // Simulate hardware setup
    // In real implementation, this would initialize the hardware interface
    glitch_engine_log("Hardware setup completed");
    return true;
}

bool glitch_engine_add_glitch_parameters(glitch_engine_t* engine, const glitch_parameters_t* params) {
    if (!engine || !params) return false;
    
    // Check if we need to expand the array
    if (engine->glitch_count >= engine->glitch_capacity) {
        uint32_t new_capacity = engine->glitch_capacity * 2;
        glitch_parameters_t* new_params = realloc(engine->glitch_params, 
                                                 sizeof(glitch_parameters_t) * new_capacity);
        if (!new_params) {
            glitch_engine_log("Failed to expand glitch parameters array");
            return false;
        }
        engine->glitch_params = new_params;
        engine->glitch_capacity = new_capacity;
    }
    
    // Copy parameters
    memcpy(&engine->glitch_params[engine->glitch_count], params, sizeof(glitch_parameters_t));
    engine->glitch_count++;
    
    glitch_engine_log("Glitch parameters added");
    return true;
}

bool glitch_engine_remove_glitch_parameters(glitch_engine_t* engine, uint32_t index) {
    if (!engine || index >= engine->glitch_count) return false;
    
    // Move remaining parameters up
    for (uint32_t i = index; i < engine->glitch_count - 1; i++) {
        engine->glitch_params[i] = engine->glitch_params[i + 1];
    }
    engine->glitch_count--;
    
    glitch_engine_log("Glitch parameters removed");
    return true;
}

bool glitch_engine_start_glitching(glitch_engine_t* engine) {
    if (!engine) return false;
    
    if (engine->is_active) {
        glitch_engine_log("Glitching already active");
        return false;
    }
    
    if (engine->glitch_count == 0) {
        glitch_engine_log("No glitch parameters configured");
        return false;
    }
    
    engine->is_active = true;
    glitch_engine_log("Glitching started");
    return true;
}

void glitch_engine_stop_glitching(glitch_engine_t* engine) {
    if (!engine) return;
    
    engine->is_active = false;
    glitch_engine_log("Glitching stopped");
}

bool glitch_engine_execute_glitch(glitch_engine_t* engine, const glitch_parameters_t* params) {
    if (!engine || !params) return false;
    
    uint64_t start_time = time(NULL);
    
    // Simulate glitch execution based on type
    switch (params->type) {
        case GLITCH_TYPE_VOLTAGE:
            glitch_engine_log("Executing voltage glitch");
            usleep(params->duration_ns / 1000);  // Convert ns to us
            break;
            
        case GLITCH_TYPE_CLOCK:
            glitch_engine_log("Executing clock glitch");
            usleep(params->duration_ns / 1000);
            break;
            
        case GLITCH_TYPE_ELECTROMAGNETIC:
            glitch_engine_log("Executing EM glitch");
            usleep(params->duration_ns / 1000);
            break;
            
        case GLITCH_TYPE_LASER_FAULT:
            glitch_engine_log("Executing laser fault injection");
            usleep(params->duration_ns / 1000);
            break;
            
        case GLITCH_TYPE_TEMPERATURE:
            glitch_engine_log("Executing temperature glitch");
            usleep(params->duration_ns / 1000);
            break;
            
        case GLITCH_TYPE_POWER_SUPPLY:
            glitch_engine_log("Executing power supply glitch");
            usleep(params->duration_ns / 1000);
            break;
            
        default:
            glitch_engine_log("Unknown glitch type");
            return false;
    }
    
    // Create result
    if (engine->glitch_result_count >= engine->glitch_result_capacity) {
        uint32_t new_capacity = engine->glitch_result_capacity * 2;
        glitch_result_t* new_results = realloc(engine->glitch_results, 
                                             sizeof(glitch_result_t) * new_capacity);
        if (!new_results) {
            glitch_engine_log("Failed to expand glitch results array");
            return false;
        }
        engine->glitch_results = new_results;
        engine->glitch_result_capacity = new_capacity;
    }
    
    glitch_result_t* result = &engine->glitch_results[engine->glitch_result_count];
    result->timestamp = time(NULL);
    memcpy(&result->parameters, params, sizeof(glitch_parameters_t));
    result->success = true;
    result->response_data = NULL;
    result->response_len = 0;
    strcpy(result->error_message, "");
    result->execution_time_us = (uint32_t)(time(NULL) - start_time);
    
    engine->glitch_result_count++;
    return true;
}

bool glitch_engine_add_fault_parameters(glitch_engine_t* engine, const fault_parameters_t* params) {
    if (!engine || !params) return false;
    
    // Check if we need to expand the array
    if (engine->fault_count >= engine->fault_capacity) {
        uint32_t new_capacity = engine->fault_capacity * 2;
        fault_parameters_t* new_params = realloc(engine->fault_params, 
                                                sizeof(fault_parameters_t) * new_capacity);
        if (!new_params) {
            glitch_engine_log("Failed to expand fault parameters array");
            return false;
        }
        engine->fault_params = new_params;
        engine->fault_capacity = new_capacity;
    }
    
    // Copy parameters
    fault_parameters_t* new_param = &engine->fault_params[engine->fault_count];
    memcpy(new_param, params, sizeof(fault_parameters_t));
    
    // Copy corruption value if present
    if (params->corruption_value && params->corruption_len > 0) {
        new_param->corruption_value = malloc(params->corruption_len);
        if (!new_param->corruption_value) {
            glitch_engine_log("Failed to allocate memory for corruption value");
            return false;
        }
        memcpy(new_param->corruption_value, params->corruption_value, params->corruption_len);
    }
    
    engine->fault_count++;
    glitch_engine_log("Fault parameters added");
    return true;
}

bool glitch_engine_remove_fault_parameters(glitch_engine_t* engine, uint32_t index) {
    if (!engine || index >= engine->fault_count) return false;
    
    // Free corruption value
    if (engine->fault_params[index].corruption_value) {
        free(engine->fault_params[index].corruption_value);
    }
    
    // Move remaining parameters up
    for (uint32_t i = index; i < engine->fault_count - 1; i++) {
        engine->fault_params[i] = engine->fault_params[i + 1];
    }
    engine->fault_count--;
    
    glitch_engine_log("Fault parameters removed");
    return true;
}

bool glitch_engine_inject_fault(glitch_engine_t* engine, const fault_parameters_t* params) {
    if (!engine || !params) return false;
    
    // Simulate fault injection based on type
    switch (params->type) {
        case FAULT_TYPE_SINGLE_BIT_FLIP:
            glitch_engine_log("Injecting single bit flip fault");
            break;
            
        case FAULT_TYPE_MULTI_BIT_FLIP:
            glitch_engine_log("Injecting multi-bit flip fault");
            break;
            
        case FAULT_TYPE_BYTE_CORRUPTION:
            glitch_engine_log("Injecting byte corruption fault");
            break;
            
        case FAULT_TYPE_INSTRUCTION_SKIP:
            glitch_engine_log("Injecting instruction skip fault");
            break;
            
        case FAULT_TYPE_BRANCH_OVERRIDE:
            glitch_engine_log("Injecting branch override fault");
            break;
            
        case FAULT_TYPE_MEMORY_CORRUPTION:
            glitch_engine_log("Injecting memory corruption fault");
            break;
            
        case FAULT_TYPE_REGISTER_CORRUPTION:
            glitch_engine_log("Injecting register corruption fault");
            break;
            
        default:
            glitch_engine_log("Unknown fault type");
            return false;
    }
    
    // Simulate fault execution
    usleep(params->duration_ns / 1000);  // Convert ns to us
    
    // Create result
    if (engine->fault_result_count >= engine->fault_result_capacity) {
        uint32_t new_capacity = engine->fault_result_capacity * 2;
        fault_result_t* new_results = realloc(engine->fault_results, 
                                            sizeof(fault_result_t) * new_capacity);
        if (!new_results) {
            glitch_engine_log("Failed to expand fault results array");
            return false;
        }
        engine->fault_results = new_results;
        engine->fault_result_capacity = new_capacity;
    }
    
    fault_result_t* result = &engine->fault_results[engine->fault_result_count];
    result->timestamp = time(NULL);
    memcpy(&result->parameters, params, sizeof(fault_parameters_t));
    result->success = true;
    result->system_response = NULL;
    result->response_len = 0;
    strcpy(result->error_message, "");
    
    engine->fault_result_count++;
    return true;
}

bool glitch_engine_add_voltage_profile(glitch_engine_t* engine, const voltage_profile_t* profile) {
    if (!engine || !profile) return false;
    
    // Check if we need to expand the array
    if (engine->voltage_profile_count >= engine->voltage_profile_capacity) {
        uint32_t new_capacity = engine->voltage_profile_capacity * 2;
        voltage_profile_t* new_profiles = realloc(engine->voltage_profiles, 
                                                 sizeof(voltage_profile_t) * new_capacity);
        if (!new_profiles) {
            glitch_engine_log("Failed to expand voltage profiles array");
            return false;
        }
        engine->voltage_profiles = new_profiles;
        engine->voltage_profile_capacity = new_capacity;
    }
    
    // Copy profile
    memcpy(&engine->voltage_profiles[engine->voltage_profile_count], profile, sizeof(voltage_profile_t));
    engine->voltage_profile_count++;
    
    glitch_engine_log("Voltage profile added");
    return true;
}

bool glitch_engine_set_active_profile(glitch_engine_t* engine, const char* profile_name) {
    if (!engine || !profile_name) return false;
    
    for (uint32_t i = 0; i < engine->voltage_profile_count; i++) {
        if (strcmp(engine->voltage_profiles[i].name, profile_name) == 0) {
            engine->active_profile = &engine->voltage_profiles[i];
            glitch_engine_log("Active profile set");
            return true;
        }
    }
    
    glitch_engine_log("Profile not found");
    return false;
}

bool glitch_engine_single_glitch(glitch_engine_t* engine, uint32_t duration_ns) {
    if (!engine) return false;
    
    if (!engine->active_profile) {
        glitch_engine_log("No active voltage profile");
        return false;
    }
    
    glitch_engine_log("Executing single glitch");
    usleep(duration_ns / 1000);  // Convert ns to us
    return true;
}

bool glitch_engine_burst_glitch(glitch_engine_t* engine, uint32_t count, uint32_t duration_ns, uint32_t interval_ns) {
    if (!engine) return false;
    
    if (!engine->active_profile) {
        glitch_engine_log("No active voltage profile");
        return false;
    }
    
    glitch_engine_log("Executing burst glitch");
    
    for (uint32_t i = 0; i < count; i++) {
        usleep(duration_ns / 1000);  // Convert ns to us
        if (i < count - 1) {
            usleep(interval_ns / 1000);  // Convert ns to us
        }
    }
    
    return true;
}

glitch_result_t* glitch_engine_get_glitch_results(glitch_engine_t* engine, uint32_t* count) {
    if (!engine || !count) return NULL;
    
    *count = engine->glitch_result_count;
    return engine->glitch_results;
}

fault_result_t* glitch_engine_get_fault_results(glitch_engine_t* engine, uint32_t* count) {
    if (!engine || !count) return NULL;
    
    *count = engine->fault_result_count;
    return engine->fault_results;
}

void glitch_engine_clear_results(glitch_engine_t* engine) {
    if (!engine) return;
    
    // Clear glitch results
    for (uint32_t i = 0; i < engine->glitch_result_count; i++) {
        if (engine->glitch_results[i].response_data) {
            free(engine->glitch_results[i].response_data);
        }
    }
    engine->glitch_result_count = 0;
    
    // Clear fault results
    for (uint32_t i = 0; i < engine->fault_result_count; i++) {
        if (engine->fault_results[i].system_response) {
            free(engine->fault_results[i].system_response);
        }
    }
    engine->fault_result_count = 0;
    
    glitch_engine_log("Results cleared");
}

bool glitch_engine_export_results(glitch_engine_t* engine, const char* filename) {
    if (!engine || !filename) return false;
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        glitch_engine_log("Failed to open file for export");
        return false;
    }
    
    fprintf(file, "{\n");
    fprintf(file, "  \"glitch_results\": [\n");
    
    for (uint32_t i = 0; i < engine->glitch_result_count; i++) {
        glitch_result_t* result = &engine->glitch_results[i];
        fprintf(file, "    {\n");
        fprintf(file, "      \"timestamp\": %llu,\n", result->timestamp);
        fprintf(file, "      \"type\": %d,\n", result->parameters.type);
        fprintf(file, "      \"timing\": %d,\n", result->parameters.timing);
        fprintf(file, "      \"voltage_level\": %.2f,\n", result->parameters.voltage_level);
        fprintf(file, "      \"duration_ns\": %u,\n", result->parameters.duration_ns);
        fprintf(file, "      \"frequency_hz\": %u,\n", result->parameters.frequency_hz);
        fprintf(file, "      \"success\": %s,\n", result->success ? "true" : "false");
        fprintf(file, "      \"execution_time_us\": %u\n", result->execution_time_us);
        fprintf(file, "    }%s\n", (i < engine->glitch_result_count - 1) ? "," : "");
    }
    
    fprintf(file, "  ],\n");
    fprintf(file, "  \"fault_results\": [\n");
    
    for (uint32_t i = 0; i < engine->fault_result_count; i++) {
        fault_result_t* result = &engine->fault_results[i];
        fprintf(file, "    {\n");
        fprintf(file, "      \"timestamp\": %llu,\n", result->timestamp);
        fprintf(file, "      \"type\": %d,\n", result->parameters.type);
        fprintf(file, "      \"target\": %d,\n", result->parameters.target);
        fprintf(file, "      \"target_address\": 0x%08X,\n", result->parameters.target_address);
        fprintf(file, "      \"bit_position\": %d,\n", result->parameters.bit_position);
        fprintf(file, "      \"duration_ns\": %u,\n", result->parameters.duration_ns);
        fprintf(file, "      \"intensity\": %.2f,\n", result->parameters.intensity);
        fprintf(file, "      \"success\": %s\n", result->success ? "true" : "false");
        fprintf(file, "    }%s\n", (i < engine->fault_result_count - 1) ? "," : "");
    }
    
    fprintf(file, "  ]\n");
    fprintf(file, "}\n");
    
    fclose(file);
    glitch_engine_log("Results exported");
    return true;
}

void glitch_engine_log(const char* message) {
    printf("[GLITCH] %s\n", message);
}

voltage_profile_t* create_voltage_profile(const char* name, float base_voltage, 
                                         float glitch_voltage, uint32_t glitch_duration_ns) {
    if (!name) return NULL;
    
    voltage_profile_t* profile = malloc(sizeof(voltage_profile_t));
    if (!profile) return NULL;
    
    memset(profile, 0, sizeof(voltage_profile_t));
    strncpy(profile->name, name, sizeof(profile->name) - 1);
    profile->base_voltage = base_voltage;
    profile->glitch_voltage = glitch_voltage;
    profile->glitch_duration_ns = glitch_duration_ns;
    profile->rise_time_ns = 1000;  // 1us
    profile->fall_time_ns = 1000;  // 1us
    profile->frequency_hz = 1000;  // 1kHz
    profile->duty_cycle = 0.1;     // 10%
    
    glitch_engine_log("Voltage profile created");
    return profile;
}

fault_parameters_t* create_bypass_fault(uint32_t target_address, uint8_t bit_position) {
    fault_parameters_t* fault = malloc(sizeof(fault_parameters_t));
    if (!fault) return NULL;
    
    memset(fault, 0, sizeof(fault_parameters_t));
    fault->type = FAULT_TYPE_SINGLE_BIT_FLIP;
    fault->target = TARGET_COMPONENT_MEMORY;
    fault->target_address = target_address;
    fault->bit_position = bit_position;
    fault->corruption_value = NULL;
    fault->corruption_len = 0;
    fault->timing_offset_ns = 0;
    fault->duration_ns = 1000;  // 1us
    fault->intensity = 0.8;
    fault->repeat_count = 1;
    fault->repeat_interval_ns = 0;
    
    glitch_engine_log("Bypass fault created");
    return fault;
}

fault_parameters_t* create_instruction_skip_fault(uint32_t target_address) {
    fault_parameters_t* fault = malloc(sizeof(fault_parameters_t));
    if (!fault) return NULL;
    
    memset(fault, 0, sizeof(fault_parameters_t));
    fault->type = FAULT_TYPE_INSTRUCTION_SKIP;
    fault->target = TARGET_COMPONENT_CPU_CORE;
    fault->target_address = target_address;
    fault->bit_position = 0;
    fault->corruption_value = NULL;
    fault->corruption_len = 0;
    fault->timing_offset_ns = 0;
    fault->duration_ns = 2000;  // 2us
    fault->intensity = 0.9;
    fault->repeat_count = 1;
    fault->repeat_interval_ns = 0;
    
    glitch_engine_log("Instruction skip fault created");
    return fault;
}

fault_parameters_t* create_branch_override_fault(uint32_t target_address) {
    fault_parameters_t* fault = malloc(sizeof(fault_parameters_t));
    if (!fault) return NULL;
    
    memset(fault, 0, sizeof(fault_parameters_t));
    fault->type = FAULT_TYPE_BRANCH_OVERRIDE;
    fault->target = TARGET_COMPONENT_CPU_CORE;
    fault->target_address = target_address;
    fault->bit_position = 0;
    fault->corruption_value = NULL;
    fault->corruption_len = 0;
    fault->timing_offset_ns = 0;
    fault->duration_ns = 1500;  // 1.5us
    fault->intensity = 0.7;
    fault->repeat_count = 1;
    fault->repeat_interval_ns = 0;
    
    glitch_engine_log("Branch override fault created");
    return fault;
}
