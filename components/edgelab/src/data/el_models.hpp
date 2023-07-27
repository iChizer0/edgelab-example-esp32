#ifndef _EL_MODELS_HPP_
#define _EL_MODELS_HPP_

#include "esp_log.h"
#include "esp_partition.h"
#include "tensorflow/lite/experimental/micro/micro_error_reporter.h"
#include "tensorflow/lite/experimental/micro/micro_interpreter.h"
​
  // Function to read the TFLite model from flash
  void*
  ReadModelDataFromFlash(size_t offset, size_t length) {
    esp_partition_t  partition;
    esp_partition_t* partition_ptr = &partition;
    // Replace "model_partition" with the appropriate partition where your model is stored.
    esp_partition_iterator_t it =
      esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "model_partition");
    if (it == NULL) {
        ESP_LOGE("TFLite", "Model partition not found");
        return nullptr;
    }
    partition            = esp_partition_get(*it);
    ​ void* model_data = malloc(length);
    if (model_data == nullptr) {
        ESP_LOGE("TFLite", "Failed to allocate memory for model data");
        return nullptr;
    }
    ​ esp_err_t err = esp_partition_read(partition_ptr, offset, model_data, length);
    if (err != ESP_OK) {
        ESP_LOGE("TFLite", "Error reading model data from flash: %s", esp_err_to_name(err));
        free(model_data);
        return nullptr;
    }
    ​ return model_data;
}
​
  // Entry point for executing the TFLite model from XIP
  void
  RunTFLiteModelFromXIP() {
    // Replace these values with the appropriate model details (offset and size in flash).
    size_t model_offset = 0x10000;
    size_t model_size   = 0x20000;
    ​
      // Read the TFLite model from flash
      void* model_data = ReadModelDataFromFlash(model_offset, model_size);
    if (model_data == nullptr) {
        ESP_LOGE("TFLite", "Failed to read model data from flash");
        return;
    }
    ​
      // Create an instance of the MicroAllocator and set it to use XIP memory
      tflite::MicroAllocator allocator;
    allocator.SetBuffer(model_data, model_size);
    ​
      // Initialize TensorFlow Lite interpreter
      tflite::MicroErrorReporter micro_error_reporter;
    tflite::ErrorReporter*       error_reporter = &micro_error_reporter;
    const tflite::Model*         model          = tflite::GetModel(error_reporter, allocator);
    if (model == nullptr) {
        ESP_LOGE("TFLite", "Failed to get TFLite model");
        free(model_data);
        return;
    }
    ​
      // Load the model and interpreter
      tflite::MicroInterpreter interpreter(model, allocator, error_reporter);
    interpreter.AllocateTensors();
    ​
      // Perform inference
      // ... (perform inference using input and output tensors)
      // ...
​
      // Cleanup
      free(model_data);
}

#endif
