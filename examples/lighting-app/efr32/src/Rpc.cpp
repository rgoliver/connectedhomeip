/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *    All rights reserved.
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

#include "Rpc.h"
#include "AppTask.h"
#include "PigweedLogger.h"
#include "PigweedLoggerMutex.h"
#include "pigweed/RpcService.h"

#include "pigweed_lighting.rpc.pb.h"
#include "platform/ConfigurationManager.h"
#include "pw_hdlc/rpc_channel.h"
#include "pw_hdlc/rpc_packets.h"
#include "pw_rpc/server.h"
#include "pw_stream/sys_io_stream.h"
#include "pw_sys_io/sys_io.h"
#include "pw_sys_io_efr32/init.h"

namespace chip {
namespace rpc {

class LightingService final : public generated::LightingService<LightingService>
{
public:
    pw::Status ButtonEvent(ServerContext & ctx, const chip_rpc_Button & request, chip_rpc_Empty & response)
    {
        GetAppTask().ButtonEventHandler(request.idx /* PB 0 or PB 1 */, request.action /* 0 =PRESSED 1= RELEASE */);
        return pw::OkStatus();
    }

    pw::Status SetState(ServerContext &, const chip_rpc_LightEnabled & request, chip_rpc_Empty & response)
    {
        LightMgr().InitiateAction(AppEvent::kEventType_Light,
                                  request.enabled ? LightingManager::ON_ACTION : LightingManager::OFF_ACTION);
        return pw::OkStatus();
    }

    pw::Status GetState(ServerContext &, const chip_rpc_Empty & request, chip_rpc_LightEnabled & response)
    {
        response.enabled = LightMgr().IsLightOn();
        return pw::OkStatus();
    }
};

class DeviceCommon final : public generated::DeviceCommon<DeviceCommon>
{
public:
    pw::Status FactoryReset(ServerContext & ctx, const chip_rpc_Empty & request, chip_rpc_Empty & response)
    {
        // TODO: Clear data from KVS
        DeviceLayer::ConfigurationMgr().InitiateFactoryReset();
        return pw::OkStatus();
    }
    pw::Status Reset(ServerContext & ctx, const chip_rpc_Empty & request, chip_rpc_Empty & response)
    {
        NVIC_SystemReset();
        // WILL NOT RETURN
        return pw::OkStatus();
    }
    pw::Status TriggerOta(ServerContext & ctx, const chip_rpc_Empty & request, chip_rpc_Empty & response)
    {
        // TODO: auto err = DeviceLayer::SoftwareUpdateMgr().CheckNow();
        return pw::Status::Unimplemented();
    }
};

namespace {
using std::byte;

#define RPC_TASK_STACK_SIZE 4096
#define RPC_TASK_PRIORITY 1
static TaskHandle_t sRpcTaskHandle;

chip::rpc::LightingService lighting_service;
chip::rpc::DeviceCommon device_common;

void RegisterServices(pw::rpc::Server & server)
{
    server.RegisterService(lighting_service);
    server.RegisterService(device_common);
}

} // namespace

void RunRpcService(void *)
{
    Start(RegisterServices, &logger_mutex);
}

int Init()
{
    int err = CHIP_ERROR_MAX;
    pw_sys_io_Init();

    // Start App task.
    if (xTaskCreate(RunRpcService, "RPC_TASK", RPC_TASK_STACK_SIZE / sizeof(StackType_t), nullptr, RPC_TASK_PRIORITY,
                    &sRpcTaskHandle) == pdPASS)
    {
        err = CHIP_NO_ERROR;
    }
    return err;
}

} // namespace rpc
} // namespace chip
