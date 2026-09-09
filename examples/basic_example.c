// Basic example for the SPI fault-campaign simulator.
//
// This example wires up the three subsystems (SPI core, injection engine,
// glitch engine), submits a small handful of synthetic SPI transactions,
// registers a single test payload + injection rule, runs one voltage-glitch
// trial, and writes the resulting structured output to disk.
//
// Nothing here drives real hardware: it exercises the in-memory data model
// end-to-end. Real hardware execution lives in pluggable backends (see
// README.md, "Backends" section).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../src/spi_framework.h"

int main(void) {
    printf("SPI Fault-Campaign Simulator — basic example\n");
    printf("============================================\n\n");

    // 1. Initialize framework
    printf("1. Initializing framework...\n");
    spi_framework_t* framework = spi_framework_init("simulator", 1000000);
    if (!framework) {
        printf("   failed to initialize framework\n");
        return 1;
    }

    // 2. Start framework
    printf("2. Starting framework...\n");
    if (!spi_framework_start(framework)) {
        printf("   failed to start framework\n");
        spi_framework_destroy(framework);
        return 1;
    }

    // 3. Submit a small synthetic SPI transcript
    printf("3. Submitting synthetic SPI transcript...\n");
    spi_transaction_t transactions[] = {
        {
            .timestamp = 0,
            .cs_pin = 0,
            .command = SPI_CMD_READ_ID,
            .address = 0,
            .data = (uint8_t[]){0x9F},
            .data_len = 1,
            .response = (uint8_t[]){0xEF, 0x40, 0x18},
            .response_len = 3,
            .duration_us = 1000
        },
        {
            .timestamp = 0,
            .cs_pin = 0,
            .command = SPI_CMD_READ_DATA,
            .address = 0x000000,
            .data = (uint8_t[]){0x03, 0x00, 0x00, 0x00},
            .data_len = 4,
            .response = (uint8_t[]){0x03, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF},
            .response_len = 8,
            .duration_us = 5000
        },
        {
            .timestamp = 0,
            .cs_pin = 0,
            .command = SPI_CMD_WRITE_ENABLE,
            .address = 0,
            .data = (uint8_t[]){0x06},
            .data_len = 1,
            .response = NULL,
            .response_len = 0,
            .duration_us = 100
        }
    };

    for (int i = 0; i < 3; i++) {
        transactions[i].timestamp = (uint64_t)time(NULL);
        if (!spi_core_add_transaction(framework->spi_core, &transactions[i])) {
            printf("   failed to add transaction %d\n", i);
        } else {
            printf("   transaction %d: CMD=0x%02X, ADDR=0x%08X\n",
                   i, transactions[i].command, transactions[i].address);
        }
    }

    // 4. Register a deterministic fault-pattern payload (0xAA repeated).
    printf("4. Registering fault-pattern payload...\n");
    payload_t* payload = create_fault_pattern_payload("aa_pattern_64", 0xAA, 64, 0x1000);
    if (payload) {
        if (injection_engine_register_payload(framework->injection_engine, payload)) {
            printf("   payload registered: %s (%u bytes)\n", payload->name, payload->size);
        } else {
            printf("   failed to register payload\n");
        }
        free(payload->data);
        free(payload);
    } else {
        printf("   failed to create payload\n");
    }

    // 5. Register an injection rule that fires on the boot-region read.
    printf("5. Registering injection rule...\n");
    injection_rule_t rule = {
        .name = "boot_region_pattern",
        .trigger = INJECTION_TRIGGER_ON_ADDRESS,
        .trigger_value.address = 0x000000,
        .mode = INJECTION_MODE_INSERT,
        .payload = NULL,
        .target_address = 0x100,
        .timing_delay_us = 0,
        .enabled = true
    };

    payload_t* found_payload = injection_engine_get_payload(framework->injection_engine,
                                                            "aa_pattern_64");
    if (found_payload) {
        rule.payload = found_payload;
        if (injection_engine_add_rule(framework->injection_engine, &rule)) {
            printf("   rule added: %s\n", rule.name);
        }
    }

    // 6. Manual injection (forced trigger, useful for unit tests).
    printf("6. Forcing one manual injection...\n");
    uint8_t test_data[] = {0x03, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    if (injection_engine_inject_manual(framework->injection_engine, "boot_region_pattern",
                                       test_data, sizeof(test_data))) {
        printf("   manual injection ok\n");
    }

    // 7. Configure a voltage-glitch profile for the simulator.
    printf("7. Configuring voltage-glitch profile...\n");
    voltage_profile_t* profile = create_voltage_profile("standard_glitch", 3.3, 0.5, 10000);
    if (profile) {
        if (glitch_engine_add_voltage_profile(framework->glitch_engine, profile)) {
            printf("   profile added: %s\n", profile->name);
            glitch_engine_set_active_profile(framework->glitch_engine, profile->name);
        }
        free(profile);
    }

    // 8. Run one single-shot glitch.
    printf("8. Running single-shot glitch...\n");
    if (glitch_engine_single_glitch(framework->glitch_engine, 10000)) {
        printf("   glitch executed\n");
    }

    // 9. Run one fault-injection trial (control-flow bypass model).
    printf("9. Running one fault-injection trial...\n");
    fault_parameters_t* fault = create_bypass_fault(0x1000, 0);
    if (fault) {
        if (glitch_engine_add_fault_parameters(framework->glitch_engine, fault)) {
            glitch_engine_inject_fault(framework->glitch_engine, fault);
            printf("   fault trial ok\n");
        }
        free(fault);
    }

    sleep(1);

    // 10. Stop + export results.
    printf("10. Stopping framework and exporting results...\n");
    spi_framework_stop(framework);
    if (spi_framework_export_all_data(framework, "basic_example")) {
        printf("    results exported to basic_example_*.json\n");
    }

    // 11. Summarize.
    printf("11. Summary:\n");
    uint32_t transaction_count = 0, injection_count = 0, glitch_count = 0, fault_count = 0;
    (void)spi_core_get_transactions(framework->spi_core, &transaction_count);
    (void)injection_engine_get_results(framework->injection_engine, &injection_count);
    (void)glitch_engine_get_glitch_results(framework->glitch_engine, &glitch_count);
    (void)glitch_engine_get_fault_results(framework->glitch_engine, &fault_count);
    printf("    spi transactions: %u\n", transaction_count);
    printf("    injection events: %u\n", injection_count);
    printf("    glitch events   : %u\n", glitch_count);
    printf("    fault events    : %u\n", fault_count);

    // 12. Cleanup.
    spi_framework_destroy(framework);
    printf("\nDone.\n");
    return 0;
}
