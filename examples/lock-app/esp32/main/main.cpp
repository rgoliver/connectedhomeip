/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "CHIPDeviceManager.h"
#include "DeviceCallbacks.h"
#include "esp_heap_caps_init.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <app/server/DataModelHandler.h>
#include <app/server/Server.h>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include <support/ErrorStr.h>

#if CONFIG_PW_RPC_ENABLED
#include "PigweedLogger.h"
#include "Rpc.h"
#endif  // CONFIG_PW_RPC_ENABLED

using namespace ::chip;
using namespace ::chip::DeviceManager;
using namespace ::chip::DeviceLayer;

static const char * TAG = "lock-app";

static DeviceCallbacks EchoCallbacks;

#if CONFIG_PW_RPC_ENABLED
static bool uartInitialised;

extern "C" void __wrap_esp_log_write(esp_log_level_t level, const char * tag, const char * format, ...)
{
    va_list v;
    va_start(v, format);
#ifndef CONFIG_LOG_DEFAULT_LEVEL_NONE
    if (uartInitialised)
    {
        char formattedMsg[CHIP_CONFIG_LOG_MESSAGE_MAX_SIZE];
        size_t len = vsnprintf(formattedMsg, sizeof formattedMsg, format, v);
        if (len >= sizeof formattedMsg)
        {
            len = sizeof formattedMsg - 1;
        }
        PigweedLogger::putString(formattedMsg, len);
    }
#endif
    va_end(v);
}
#endif  // CONFIG_PW_RPC_ENABLED

extern "C" void app_main()
{
    int err = 0;
    // Initialize the ESP NVS layer.
    err = nvs_flash_init();
    if (err != CHIP_NO_ERROR)
    {
        ESP_LOGE(TAG, "nvs_flash_init() failed: %s", ErrorStr(err));
        return;
    }

#if CONFIG_PW_RPC_ENABLED
    chip::rpc::Init();
    uartInitialised = true;
#endif  // CONFIG_PW_RPC_ENABLED

    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "chip-esp32-lock-example starting");
    ESP_LOGI(TAG, "==================================================");

    CHIPDeviceManager & deviceMgr = CHIPDeviceManager::GetInstance();

    err = deviceMgr.Init(&EchoCallbacks);
    if (err != CHIP_NO_ERROR)
    {
        ESP_LOGE(TAG, "device.Init() failed: %s", ErrorStr(err));
        return;
    }

    InitServer();

    while (true)
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
