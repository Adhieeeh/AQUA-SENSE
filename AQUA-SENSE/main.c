/**
 * @file main.c
 * @brief AQUA-SENSE: IoT-Enabled Ultrasonic Signal Processing Framework
 * @details Implements a low-overhead, resource-constrained firmware architecture 
 * using a Digital Windowed Moving Average Filter to eliminate environmental 
 * noise for real-time urban flood depth telemetry.
 * @author Adhithya S U
 * @date June 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#define WINDOW_SIZE 12
#define SOUND_VELOCITY_343MS 343.0f 
// System Configuration Constants
const float TRANSDUCER_BASE_HEIGHT_CM = 200.0f; 
const float NOISE_THRESHOLD_PERCENT = 0.15f;    

float dataWindow[WINDOW_SIZE];
int bufferIndex = 0;
bool isBufferWarmedUp = false;


void updateBuffer(float newSample) {
    dataWindow[bufferIndex] = newSample;
    bufferIndex = (bufferIndex + 1) % WINDOW_SIZE;
    if (bufferIndex == 0) {
        isBufferWarmedUp = true;
    }
}


void transmitMQTT(float validatedToF, float calculatedDepth) {
    printf("   >>> [MQTT TELEMETRY]: Serializing uplink payload...\n");
    printf("   >>> [MQTT STRING]: {\"tof_ms\": %.4f, \"depth_cm\": %.2f}\n\n", validatedToF * 1000.0f, calculatedDepth);
}


void allocateVisualAlert(float depth) {
    printf("   >>> [LOCAL ALERT]: ");
    if (depth < 10.0f) {
        printf("[GREEN INDICATOR] - Safe roadway environment. Normal commuting.\n");
    } else if (depth >= 10.0f && depth <= 20.0f) {
        printf("[YELLOW INDICATOR] - Cautionary status. High risk for low-clearance wheelbases.\n");
    } else {
        printf("[RED STROBE + BUZZER] - Critical Danger! Complete road closure warning.\n");
    }
}


void processSignalData(float currentToF) {
    float runningSum = 0.0f;
    int samplesToCount = isBufferWarmedUp ? WINDOW_SIZE : (bufferIndex == 0 ? 1 : bufferIndex);

    
    for (int i = 0; i < samplesToCount; i++) {
        runningSum += dataWindow[i];
    }
    float rollingMean = runningSum / (float)samplesToCount;

    printf("Incoming ToF Sample: %.6f s | Rolling Window Baseline Mean: %.6f s\n", currentToF, rollingMean);

    
    if (isBufferWarmedUp && (fabsf(currentToF - rollingMean) > (NOISE_THRESHOLD_PERCENT * rollingMean))) {
        printf("   ⚠️ [REJECTED]: Transient noise variation exceeded 15%% threshold (Raindrop/Obstruction scatter).\n\n");
        return; 
    }

    printf("   ✅ [ACCEPTED]: Data point matches mathematical baseline trend.\n");
    
    
    updateBuffer(currentToF);

    
    float calculatedDistanceToWater = (SOUND_VELOCITY_343MS * currentToF * 100.0f) / 2.0f; // in cm
    float localizedFloodDepth = TRANSDUCER_BASE_HEIGHT_CM - calculatedDistanceToWater;

    if (localizedFloodDepth < 0.0f) localizedFloodDepth = 0.0f; // Guard bands

    printf("   >>> Calculated Air Gap Distance: %.2f cm | Localized Flood Depth: %.2f cm\n", 
           calculatedDistanceToWater, localizedFloodDepth);

    
    allocateVisualAlert(localizedFloodDepth);
    transmitMQTT(currentToF, localizedFloodDepth);
}

/**
 * @brief Application entry point. Feeds mock data streams to evaluate algorithm logic.
 */
int main() {
    printf("=====================================================================\n");
    printf("   AQUA-SENSE CORE DIGITAL CONDITIONING FIRMWARE SIMULATION          \n");
    printf("=====================================================================\n\n");

    
    float baselineDryToF = (2.0f * 2.0f) / 343.0f; 
    for (int i = 0; i < WINDOW_SIZE; i++) {
        updateBuffer(baselineDryToF);
    }

    // --- STREAM TEST SEQUENCE ---

    
    printf("[STREAM TIME: T+1] Entering gradual flash flood dataset point...\n");
    processSignalData(0.010787f);

    
    printf("[STREAM TIME: T+2] Raindrop artifact intersecting acoustic propagation path...\n");
    processSignalData(0.005000f); 

   
    printf("[STREAM TIME: T+3] Entering catastrophic flooding dataset trend...\n");
    processSignalData(0.010204f);

    printf("=====================================================================\n");
    printf("   SIMULATION RUN COMPLETE                                           \n");
    printf("=====================================================================\n");

    return 0;
}