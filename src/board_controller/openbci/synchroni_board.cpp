#include "inc/synchroni_board.h"
#include <stdint.h>
#include <string.h>
#include "timestamp.h"
#include <string.h>
#include "inc/board.h"
#include "inc/board_controller.h"

// Define Synchroni-specific characteristics
#define SYNCHRONI_WRITE_CHAR "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX" // Replace with actual UUID
#define SYNCHRONI_NOTIFY_CHAR "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX" // Replace with actual UUID
#define SYNCHRONI_SOFTWARE_REVISION "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX" // Replace with actual UUID

// Static callback functions should refer to SynchroniBoard

static void synchroni_adapter_on_scan_start(simpleble_adapter_t adapter, void *board)
{
    ((SynchroniBoard *)(board))->adapter_on_scan_start(adapter);
}

static void synchroni_adapter_on_scan_stop(simpleble_adapter_t adapter, void *board)
{
    ((SynchroniBoard *)(board))->adapter_on_scan_stop(adapter);
}

static void synchroni_adapter_on_scan_found(simpleble_adapter_t adapter, simpleble_peripheral_t peripheral, void *board)
{
    ((SynchroniBoard *)(board))->adapter_on_scan_found(adapter, peripheral);
}

static void synchroni_read_notifications(simpleble_uuid_t service, simpleble_uuid_t characteristic,
    uint8_t *data, size_t size, void *board)
{
    ((SynchroniBoard *)(board))->read_data(service, characteristic, data, size);
}

SynchroniBoard::SynchroniBoard(struct BrainFlowInputParams params)
    : BLELibBoard((int)BoardIds::SYNCHRONI_TRIO, params), synchroni_adapter(nullptr), synchroni_peripheral(nullptr)
{
    // Constructor initialization
    initialized = false;
    is_streaming = false;
    start_command = "start"; // Replace with the actual start command for Synchroni
    stop_command = "stop"; // Replace with the actual stop command for Synchroni
    firmware = 0;
}

SynchroniBoard::~SynchroniBoard()
{
    skip_logs = true;
    release_session();
}

int SynchroniBoard::prepare_session()
{
    if (initialized)
    {
        safe_logger(spdlog::level::info, "Session is already prepared");
        return (int)BrainFlowExitCodes::STATUS_OK;
    }
    
    if (params.timeout < 1)
    {
        params.timeout = 5;
    }
    safe_logger(spdlog::level::info, "Use timeout for discovery: {}", params.timeout);
    
    // Initialize any specific DLLs or other loaders here
    if (!init_dll_loader())
    {
        safe_logger(spdlog::level::err, "Failed to init dll_loader");
        return (int)BrainFlowExitCodes::GENERAL_ERROR;
    }
    
    // Setup Bluetooth adapter and scan for the device
    size_t adapter_count = simpleble_adapter_get_count();
    if (adapter_count == 0)
    {
        safe_logger(spdlog::level::err, "No BLE adapters found");
        return (int)BrainFlowExitCodes::UNABLE_TO_OPEN_PORT_ERROR;
    }
    
    synchroni_adapter = simpleble_adapter_get_handle(0); // Set the synchroni_adapter
    if (synchroni_adapter == NULL)
    {
        safe_logger(spdlog::level::err, "Adapter is NULL");
        return (int)BrainFlowExitCodes::UNABLE_TO_OPEN_PORT_ERROR;
    }
    
    simpleble_adapter_set_callback_on_scan_start(
        synchroni_adapter, ::synchroni_adapter_on_scan_start, (void *)this);
    simpleble_adapter_set_callback_on_scan_stop(
        synchroni_adapter, ::synchroni_adapter_on_scan_stop, (void *)this);
    simpleble_adapter_set_callback_on_scan_found(
        synchroni_adapter, ::synchroni_adapter_on_scan_found, (void *)this);
    
    // Start scanning for devices
    simpleble_adapter_scan_start(synchroni_adapter);
    int res = (int)BrainFlowExitCodes::STATUS_OK;
    std::unique_lock<std::mutex> lk(m);
    auto sec = std::chrono::seconds(1);
    if (cv.wait_for(
            lk, params.timeout * sec, [this] { return this->synchroni_peripheral != NULL; }))
    {
        safe_logger(spdlog::level::info, "Found Synchroni device");
    }
    else
    {
        safe_logger(spdlog::level::err, "Failed to find Synchroni device");
        res = (int)BrainFlowExitCodes::BOARD_NOT_READY_ERROR;
    }
    
    simpleble_adapter_scan_stop(synchroni_adapter);
    if (res == (int)BrainFlowExitCodes::STATUS_OK)
    {
        // Connect to the device and discover its services
        for (int i = 0; i < 3; i++)
        {
            if (simpleble_peripheral_connect(synchroni_peripheral) == SIMPLEBLE_SUCCESS)
            {
                safe_logger(spdlog::level::info, "Connected to Synchroni device");
                res = (int)BrainFlowExitCodes::STATUS_OK;
                break;
            }
            else
            {
                safe_logger(spdlog::level::warn, "Failed to connect to Synchroni device: {}/3", i);
                res = (int)BrainFlowExitCodes::BOARD_NOT_READY_ERROR;
#ifdef _WIN32
                Sleep(1000);
#else
                sleep(1);
#endif
            }
        }
    }
    
    // Discover characteristics and setup notifications or write capabilities
    if (res == (int)BrainFlowExitCodes::STATUS_OK)
    {
        size_t services_count = simpleble_peripheral_services_count(synchroni_peripheral);
        for (size_t i = 0; i < services_count; i++)
        {
            simpleble_service_t service;
            if (simpleble_peripheral_services_get(synchroni_peripheral, i, &service) != SIMPLEBLE_SUCCESS)
            {
                safe_logger(spdlog::level::err, "Failed to get service");
                res = (int)BrainFlowExitCodes::BOARD_NOT_READY_ERROR;
            }

            safe_logger(spdlog::level::trace, "Found service {}", service.uuid.value);
            // Discover the characteristics and set up the write and notify characteristics as needed
        }
    }
    
    if (res == (int)BrainFlowExitCodes::STATUS_OK)
    {
        initialized = true;
    }
    else
    {
        release_session();
    }
    return res;
}

int SynchroniBoard::start_stream(int buffer_size, const char *streamer_params)
{
    // Implementation for starting the data stream
    // Use similar logic to what you've seen in GanglionNative or adapt based on the Synchroni protocol
    return (int)BrainFlowExitCodes::STATUS_OK;
}
