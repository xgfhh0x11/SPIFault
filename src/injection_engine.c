#include "injection_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_CAPACITY 100

injection_engine_t* injection_engine_init(void) {
    injection_engine_t* engine = malloc(sizeof(injection_engine_t));
    if (!engine) {
        injection_engine_log("Failed to allocate memory for injection engine");
        return NULL;
    }
    
    memset(engine, 0, sizeof(injection_engine_t));
    
    // Initialize rules array
    engine->rule_capacity = DEFAULT_CAPACITY;
    engine->rules = malloc(sizeof(injection_rule_t) * engine->rule_capacity);
    if (!engine->rules) {
        injection_engine_log("Failed to allocate memory for rules");
        free(engine);
        return NULL;
    }
    engine->rule_count = 0;
    
    // Initialize results array
    engine->result_capacity = DEFAULT_CAPACITY;
    engine->results = malloc(sizeof(injection_result_t) * engine->result_capacity);
    if (!engine->results) {
        injection_engine_log("Failed to allocate memory for results");
        free(engine->rules);
        free(engine);
        return NULL;
    }
    engine->result_count = 0;
    
    // Initialize payloads array
    engine->payload_capacity = DEFAULT_CAPACITY;
    engine->payloads = malloc(sizeof(payload_t) * engine->payload_capacity);
    if (!engine->payloads) {
        injection_engine_log("Failed to allocate memory for payloads");
        free(engine->rules);
        free(engine->results);
        free(engine);
        return NULL;
    }
    engine->payload_count = 0;
    
    engine->is_active = false;
    injection_engine_log("Injection engine initialized");
    return engine;
}

void injection_engine_destroy(injection_engine_t* engine) {
    if (!engine) return;
    
    // Free rules
    if (engine->rules) {
        free(engine->rules);
    }
    
    // Free results
    if (engine->results) {
        for (uint32_t i = 0; i < engine->result_count; i++) {
            if (engine->results[i].original_data) {
                free(engine->results[i].original_data);
            }
            if (engine->results[i].injected_data) {
                free(engine->results[i].injected_data);
            }
        }
        free(engine->results);
    }
    
    // Free payloads
    if (engine->payloads) {
        for (uint32_t i = 0; i < engine->payload_count; i++) {
            if (engine->payloads[i].data) {
                free(engine->payloads[i].data);
            }
        }
        free(engine->payloads);
    }
    
    free(engine);
    injection_engine_log("Injection engine destroyed");
}

bool injection_engine_add_rule(injection_engine_t* engine, const injection_rule_t* rule) {
    if (!engine || !rule) return false;
    
    // Check if we need to expand the array
    if (engine->rule_count >= engine->rule_capacity) {
        uint32_t new_capacity = engine->rule_capacity * 2;
        injection_rule_t* new_rules = realloc(engine->rules, 
                                             sizeof(injection_rule_t) * new_capacity);
        if (!new_rules) {
            injection_engine_log("Failed to expand rules array");
            return false;
        }
        engine->rules = new_rules;
        engine->rule_capacity = new_capacity;
    }
    
    // Copy rule
    memcpy(&engine->rules[engine->rule_count], rule, sizeof(injection_rule_t));
    engine->rule_count++;
    
    injection_engine_log("Injection rule added");
    return true;
}

// NOTE: not called from any path in this repository (public API only —
// no caller in src/ or examples/).
bool injection_engine_remove_rule(injection_engine_t* engine, const char* rule_name) {
    if (!engine || !rule_name) return false;
    
    for (uint32_t i = 0; i < engine->rule_count; i++) {
        if (strcmp(engine->rules[i].name, rule_name) == 0) {
            // Move remaining rules up
            for (uint32_t j = i; j < engine->rule_count - 1; j++) {
                engine->rules[j] = engine->rules[j + 1];
            }
            engine->rule_count--;
            injection_engine_log("Injection rule removed");
            return true;
        }
    }
    
    injection_engine_log("Rule not found");
    return false;
}

// NOTE: not called from any path in this repository (public API only —
// no caller in src/ or examples/).
bool injection_engine_enable_rule(injection_engine_t* engine, const char* rule_name) {
    if (!engine || !rule_name) return false;
    
    for (uint32_t i = 0; i < engine->rule_count; i++) {
        if (strcmp(engine->rules[i].name, rule_name) == 0) {
            engine->rules[i].enabled = true;
            injection_engine_log("Rule enabled");
            return true;
        }
    }
    
    return false;
}

// NOTE: not called from any path in this repository (public API only —
// no caller in src/ or examples/).
bool injection_engine_disable_rule(injection_engine_t* engine, const char* rule_name) {
    if (!engine || !rule_name) return false;
    
    for (uint32_t i = 0; i < engine->rule_count; i++) {
        if (strcmp(engine->rules[i].name, rule_name) == 0) {
            engine->rules[i].enabled = false;
            injection_engine_log("Rule disabled");
            return true;
        }
    }
    
    return false;
}

bool injection_engine_start_injection(injection_engine_t* engine) {
    if (!engine) return false;
    
    if (engine->is_active) {
        injection_engine_log("Injection already active");
        return false;
    }
    
    engine->is_active = true;
    injection_engine_log("Injection started");
    return true;
}

void injection_engine_stop_injection(injection_engine_t* engine) {
    if (!engine) return;
    
    engine->is_active = false;
    injection_engine_log("Injection stopped");
}

bool injection_engine_inject_manual(injection_engine_t* engine, const char* rule_name, 
                                   const uint8_t* data, uint16_t len) {
    if (!engine || !rule_name || !data) return false;
    
    // Find the rule
    injection_rule_t* rule = NULL;
    for (uint32_t i = 0; i < engine->rule_count; i++) {
        if (strcmp(engine->rules[i].name, rule_name) == 0) {
            rule = &engine->rules[i];
            break;
        }
    }
    
    if (!rule) {
        injection_engine_log("Rule not found");
        return false;
    }
    
    if (!rule->enabled) {
        injection_engine_log("Rule disabled");
        return false;
    }
    
    // Apply injection
    uint8_t* injected_data = NULL;
    uint16_t injected_len = 0;
    
    if (!injection_engine_apply_injection(rule, data, len, &injected_data, &injected_len)) {
        injection_engine_log("Injection failed");
        return false;
    }
    
    // Create result
    if (engine->result_count >= engine->result_capacity) {
        uint32_t new_capacity = engine->result_capacity * 2;
        injection_result_t* new_results = realloc(engine->results, 
                                                 sizeof(injection_result_t) * new_capacity);
        if (!new_results) {
            injection_engine_log("Failed to expand results array");
            if (injected_data) free(injected_data);
            return false;
        }
        engine->results = new_results;
        engine->result_capacity = new_capacity;
    }
    
    injection_result_t* result = &engine->results[engine->result_count];
    result->timestamp = time(NULL);
    strncpy(result->rule_name, rule_name, sizeof(result->rule_name) - 1);
    
    // Copy original data
    result->original_data = malloc(len);
    if (!result->original_data) {
        injection_engine_log("Failed to allocate memory for original data");
        if (injected_data) free(injected_data);
        return false;
    }
    memcpy(result->original_data, data, len);
    result->original_len = len;
    
    // Copy injected data
    result->injected_data = injected_data;
    result->injected_len = injected_len;
    result->success = true;
    strcpy(result->error_message, "");
    
    engine->result_count++;
    injection_engine_log("Manual injection completed");
    return true;
}

bool injection_engine_register_payload(injection_engine_t* engine, const payload_t* payload) {
    if (!engine || !payload) return false;
    
    // Check if we need to expand the array
    if (engine->payload_count >= engine->payload_capacity) {
        uint32_t new_capacity = engine->payload_capacity * 2;
        payload_t* new_payloads = realloc(engine->payloads, 
                                        sizeof(payload_t) * new_capacity);
        if (!new_payloads) {
            injection_engine_log("Failed to expand payloads array");
            return false;
        }
        engine->payloads = new_payloads;
        engine->payload_capacity = new_capacity;
    }
    
    // Copy payload
    payload_t* new_payload = &engine->payloads[engine->payload_count];
    memcpy(new_payload, payload, sizeof(payload_t));
    
    // Copy payload data
    if (payload->data && payload->size > 0) {
        new_payload->data = malloc(payload->size);
        if (!new_payload->data) {
            injection_engine_log("Failed to allocate memory for payload data");
            return false;
        }
        memcpy(new_payload->data, payload->data, payload->size);
    }
    
    engine->payload_count++;
    injection_engine_log("Payload registered");
    return true;
}

payload_t* injection_engine_get_payload(injection_engine_t* engine, const char* name) {
    if (!engine || !name) return NULL;
    
    for (uint32_t i = 0; i < engine->payload_count; i++) {
        if (strcmp(engine->payloads[i].name, name) == 0) {
            return &engine->payloads[i];
        }
    }
    
    return NULL;
}

injection_result_t* injection_engine_get_results(injection_engine_t* engine, uint32_t* count) {
    if (!engine || !count) return NULL;
    
    *count = engine->result_count;
    return engine->results;
}

// NOTE: not called from any path in this repository (public API only —
// no caller in src/ or examples/).
void injection_engine_clear_results(injection_engine_t* engine) {
    if (!engine) return;
    
    for (uint32_t i = 0; i < engine->result_count; i++) {
        if (engine->results[i].original_data) {
            free(engine->results[i].original_data);
        }
        if (engine->results[i].injected_data) {
            free(engine->results[i].injected_data);
        }
    }
    engine->result_count = 0;
    injection_engine_log("Results cleared");
}

bool injection_engine_export_results(injection_engine_t* engine, const char* filename) {
    if (!engine || !filename) return false;
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        injection_engine_log("Failed to open file for export");
        return false;
    }
    
    fprintf(file, "{\n");
    fprintf(file, "  \"injection_results\": [\n");
    
    for (uint32_t i = 0; i < engine->result_count; i++) {
        injection_result_t* result = &engine->results[i];
        fprintf(file, "    {\n");
        fprintf(file, "      \"timestamp\": %llu,\n", result->timestamp);
        fprintf(file, "      \"rule_name\": \"%s\",\n", result->rule_name);
        fprintf(file, "      \"original_len\": %d,\n", result->original_len);
        fprintf(file, "      \"injected_len\": %d,\n", result->injected_len);
        fprintf(file, "      \"success\": %s,\n", result->success ? "true" : "false");
        fprintf(file, "      \"error_message\": \"%s\"\n", result->error_message);
        fprintf(file, "    }%s\n", (i < engine->result_count - 1) ? "," : "");
    }
    
    fprintf(file, "  ]\n");
    fprintf(file, "}\n");
    
    fclose(file);
    injection_engine_log("Results exported");
    return true;
}

// NOTE: not called from any path in this repository (public API only —
// no caller in src/ or examples/).
payload_t* create_raw_payload(const char* name, const uint8_t* bytes,
                              uint32_t length, uint32_t target_address) {
    if (!name || !bytes || length == 0) return NULL;

    payload_t* payload = malloc(sizeof(payload_t));
    if (!payload) return NULL;

    memset(payload, 0, sizeof(payload_t));
    strncpy(payload->name, name, sizeof(payload->name) - 1);
    payload->type = PAYLOAD_TYPE_RAW;
    payload->target_address = target_address;
    payload->timing = INJECTION_TIMING_ALIGNED;
    payload->noise_level = 0;

    payload->data = malloc(length);
    if (!payload->data) {
        free(payload);
        return NULL;
    }
    memcpy(payload->data, bytes, length);
    payload->size = length;

    injection_engine_log("Raw payload created");
    return payload;
}

// NOTE: not called from any path in this repository (public API only —
// no caller in src/ or examples/).
payload_t* create_firmware_patch(const char* name, const uint8_t* original_data,
                                uint32_t original_len, const uint8_t* patched_data,
                                uint32_t patched_len, uint32_t target_address) {
    (void)original_data;
    (void)original_len;
    if (!name || !patched_data || patched_len == 0) return NULL;

    payload_t* payload = malloc(sizeof(payload_t));
    if (!payload) return NULL;

    memset(payload, 0, sizeof(payload_t));
    strncpy(payload->name, name, sizeof(payload->name) - 1);
    payload->type = PAYLOAD_TYPE_FIRMWARE_PATCH;
    payload->target_address = target_address;
    payload->timing = INJECTION_TIMING_ALIGNED;
    payload->noise_level = 0;

    payload->data = malloc(patched_len);
    if (!payload->data) {
        free(payload);
        return NULL;
    }

    memcpy(payload->data, patched_data, patched_len);
    payload->size = patched_len;

    injection_engine_log("Firmware patch created");
    return payload;
}

// NOTE: not called from any path in this repository (public API only —
// no caller in src/ or examples/).
payload_t* create_boot_image_payload(const char* name, const uint8_t* image_data,
                                     uint32_t image_len) {
    if (!name || !image_data || image_len == 0) return NULL;

    payload_t* payload = malloc(sizeof(payload_t));
    if (!payload) return NULL;

    memset(payload, 0, sizeof(payload_t));
    strncpy(payload->name, name, sizeof(payload->name) - 1);
    payload->type = PAYLOAD_TYPE_BOOT_IMAGE;
    payload->target_address = 0x000000;  // boot region start
    payload->timing = INJECTION_TIMING_ALIGNED;
    payload->noise_level = 0;

    // Frame the image with a 4-byte little-endian length prefix so the
    // simulator can recover the original length unambiguously.
    const uint32_t header_size = 4;
    const uint32_t total_size = header_size + image_len;

    payload->data = malloc(total_size);
    if (!payload->data) {
        free(payload);
        return NULL;
    }

    payload->data[0] = (image_len >> 0)  & 0xFF;
    payload->data[1] = (image_len >> 8)  & 0xFF;
    payload->data[2] = (image_len >> 16) & 0xFF;
    payload->data[3] = (image_len >> 24) & 0xFF;
    memcpy(payload->data + header_size, image_data, image_len);

    payload->size = total_size;
    injection_engine_log("Boot image payload created");
    return payload;
}

payload_t* create_fault_pattern_payload(const char* name, uint8_t pattern_byte,
                                        uint32_t length, uint32_t target_address) {
    if (!name || length == 0) return NULL;

    payload_t* payload = malloc(sizeof(payload_t));
    if (!payload) return NULL;

    memset(payload, 0, sizeof(payload_t));
    strncpy(payload->name, name, sizeof(payload->name) - 1);
    payload->type = PAYLOAD_TYPE_FAULT_PATTERN;
    payload->target_address = target_address;
    payload->timing = INJECTION_TIMING_ALIGNED;
    payload->noise_level = 0;

    payload->data = malloc(length);
    if (!payload->data) {
        free(payload);
        return NULL;
    }
    memset(payload->data, pattern_byte, length);
    payload->size = length;

    injection_engine_log("Fault pattern payload created");
    return payload;
}

void injection_engine_log(const char* message) {
    printf("[INJECTION] %s\n", message);
}

// Bounded byte-pattern search. `strstr`/`strchr`-style scanning requires a
// NUL-terminated haystack, but `data` here is a length-delimited byte
// buffer with no NUL guarantee, so we search at most `haystack_len` bytes
// instead of scanning until a NUL byte is found (which could run past the
// end of the buffer).
static const uint8_t* bounded_search(const uint8_t* haystack, size_t haystack_len,
                                      const char* needle, size_t needle_len) {
    if (needle_len == 0 || needle_len > haystack_len) return NULL;

    for (size_t i = 0; i + needle_len <= haystack_len; i++) {
        if (memcmp(haystack + i, needle, needle_len) == 0) {
            return haystack + i;
        }
    }
    return NULL;
}

// NOTE: not called from any path in this repository (public API only —
// no caller in src/ or examples/). The address/pattern-trigger bounds
// fixes above apply to this function regardless, since it is exported
// via injection_engine.h for external callers.
bool injection_engine_process_data(injection_engine_t* engine, const uint8_t* data, uint16_t len) {
    if (!engine || !data || len == 0) return false;
    
    // Process data through active rules
    for (uint32_t i = 0; i < engine->rule_count; i++) {
        injection_rule_t* rule = &engine->rules[i];
        if (!rule->enabled) continue;
        
        // Check trigger conditions
        bool should_trigger = false;
        
        switch (rule->trigger) {
            case INJECTION_TRIGGER_ON_COMMAND:
                if (len > 0 && data[0] == rule->trigger_value.command) {
                    should_trigger = true;
                }
                break;
            case INJECTION_TRIGGER_ON_ADDRESS:
                if (len >= 5) {
                    uint32_t address = (data[1] << 24) | (data[2] << 16) |
                                      (data[3] << 8) | data[4];
                    if (address == rule->trigger_value.address) {
                        should_trigger = true;
                    }
                }
                break;
            case INJECTION_TRIGGER_ON_DATA_PATTERN:
                // Simple pattern matching, bounded to `len` bytes of `data`
                // and to the (not guaranteed NUL-terminated) pattern buffer,
                // instead of strstr() scanning past either end for a NUL.
                {
                    const char* pattern = rule->trigger_value.pattern;
                    size_t pattern_maxlen = sizeof(rule->trigger_value.pattern);
                    size_t pattern_len = 0;
                    while (pattern_len < pattern_maxlen && pattern[pattern_len] != '\0') {
                        pattern_len++;
                    }
                    if (bounded_search(data, len, pattern, pattern_len) != NULL) {
                        should_trigger = true;
                    }
                }
                break;
            default:
                break;
        }
        
        if (should_trigger) {
            // Apply injection
            uint8_t* injected_data = NULL;
            uint16_t injected_len = 0;
            
            if (injection_engine_apply_injection(rule, data, len, &injected_data, &injected_len)) {
                injection_engine_log("Injection applied");
                if (injected_data) free(injected_data);
            }
        }
    }
    
    return true;
}

bool injection_engine_apply_injection(const injection_rule_t* rule, const uint8_t* original_data,
                                     uint16_t original_len, uint8_t** injected_data,
                                     uint16_t* injected_len) {
    if (!rule || !original_data || !injected_data || !injected_len) return false;
    
    *injected_data = NULL;
    *injected_len = 0;
    
    if (!rule->payload) return false;
    
    switch (rule->mode) {
        case INJECTION_MODE_PASSIVE:
            // No injection
            return true;
            
        case INJECTION_MODE_ACTIVE:
            // Append payload
            if ((uint32_t)original_len + rule->payload->size > UINT16_MAX) return false;
            *injected_len = (uint16_t)(original_len + rule->payload->size);
            *injected_data = malloc(*injected_len);
            if (!*injected_data) return false;

            memcpy(*injected_data, original_data, original_len);
            memcpy(*injected_data + original_len, rule->payload->data, rule->payload->size);
            break;

        case INJECTION_MODE_REPLACE:
            // Replace with payload
            if (rule->payload->size > UINT16_MAX) return false;
            *injected_len = (uint16_t)rule->payload->size;
            *injected_data = malloc(*injected_len);
            if (!*injected_data) return false;

            memcpy(*injected_data, rule->payload->data, rule->payload->size);
            break;

        case INJECTION_MODE_INSERT:
            // Insert payload at target address
            if ((uint32_t)original_len + rule->payload->size > UINT16_MAX) return false;
            if (rule->target_address < original_len) {
                *injected_len = (uint16_t)(original_len + rule->payload->size);
                *injected_data = malloc(*injected_len);
                if (!*injected_data) return false;

                memcpy(*injected_data, original_data, rule->target_address);
                memcpy(*injected_data + rule->target_address, rule->payload->data, rule->payload->size);
                memcpy(*injected_data + rule->target_address + rule->payload->size,
                       original_data + rule->target_address,
                       original_len - rule->target_address);
            } else {
                // Append if target address is beyond original data
                *injected_len = (uint16_t)(original_len + rule->payload->size);
                *injected_data = malloc(*injected_len);
                if (!*injected_data) return false;

                memcpy(*injected_data, original_data, original_len);
                memcpy(*injected_data + original_len, rule->payload->data, rule->payload->size);
            }
            break;
            
        case INJECTION_MODE_MODIFY:
            // Modify specific bytes
            *injected_len = original_len;
            *injected_data = malloc(*injected_len);
            if (!*injected_data) return false;
            
            memcpy(*injected_data, original_data, original_len);
            
            if (rule->target_address < original_len) {
                uint32_t modify_len = (rule->payload->size < (original_len - rule->target_address)) ?
                                     rule->payload->size : (original_len - rule->target_address);
                memcpy(*injected_data + rule->target_address, rule->payload->data, modify_len);
            }
            break;
            
        default:
            return false;
    }
    
    return true;
}
