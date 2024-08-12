#pragma once

#include "board.h"
#include "board_controller.h"
#include "ble_lib_board.h"
#include <condition_variable> // Include for std::condition_variable
#include <mutex> // Include for std::mutex
#include <string> // Include for std::string

class SynchroniBoard : public BLELibBoard
{
private:
    simpleble_adapter_t synchroni_adapter;         // BLE adapter handle
    simpleble_peripheral_t synchroni_peripheral;   // BLE peripheral handle
    bool initialized;                              // Flag to check if session is initialized
    bool is_streaming;                             // Flag to check if streaming is active
    std::string start_command;                     // Command to start streaming
    std::string stop_command;                      // Command to stop streaming
    int firmware;                                  // Firmware version

    std::condition_variable cv;                    // Condition variable for synchronization
    std::mutex m;                                  // Mutex to protect shared data

public:
    SynchroniBoard(struct BrainFlowInputParams params);
    ~SynchroniBoard();

    int prepare_session();                         // Prepare session for the device
    int start_stream(int buffer_size, const char *streamer_params);  // Start data streaming
    int stop_stream();                             // Stop data streaming
    int release_session();                         // Release resources and end session
    int config_board(std::string config, std::string &response); // Configure board with response
    int config_board(std::string config);          // Configure board without response

    // Callbacks for BLE events
    void adapter_on_scan_start(simpleble_adapter_t adapter);  // Handle BLE scan start
    void adapter_on_scan_stop(simpleble_adapter_t adapter);   // Handle BLE scan stop
    void adapter_on_scan_found(simpleble_adapter_t adapter, simpleble_peripheral_t peripheral); // Handle found device during scan

    // Method to handle data received from the device
    void read_data(simpleble_uuid_t service, simpleble_uuid_t characteristic, uint8_t *data, size_t size);
};
