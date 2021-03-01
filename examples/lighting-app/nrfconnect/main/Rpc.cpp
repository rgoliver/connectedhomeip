/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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
#include "LightingManager.h"
#include "PigweedLogger.h"
#include "PigweedLoggerMutex.h"
#include "pigweed/RpcService.h"

#include "main/pigweed_lighting.rpc.pb.h"
#include "pw_hdlc/rpc_channel.h"
#include "pw_hdlc/rpc_packets.h"
#include "pw_rpc/server.h"
#include "pw_stream/sys_io_stream.h"
#include "pw_sys_io/sys_io.h"
#include "pw_sys_io_nrfconnect/init.h"

#include <array>
#include <kernel.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(app);

namespace chip {
namespace rpc {

class LightingService final : public generated::LightingService<LightingService>
{
public:
    pw::Status ButtonEvent(ServerContext & ctx, const chip_rpc_Button & request, chip_rpc_Empty & response)
    {
        GetAppTask().ButtonEventHandler(request.action << request.idx /* button_state */, 1 << request.idx /* has_changed */);
        return pw::OkStatus();
    }

    pw::Status SetState(ServerContext &, const chip_rpc_LightEnabled & request, chip_rpc_Empty & response)
    {
        LightingMgr().InitiateAction(request.enabled ? LightingManager::ON_ACTION : LightingManager::OFF_ACTION,
                                     AppEvent::kEventType_Button, 0, NULL);
        return pw::OkStatus();
    }

    pw::Status GetState(ServerContext &, const chip_rpc_Empty & request, chip_rpc_LightEnabled & response)
    {
        response.enabled = LightingMgr().IsTurnedOn();
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

constexpr size_t kRpcTaskSize = 4096;
constexpr int kRpcPriority    = 5;

K_THREAD_STACK_DEFINE(rpc_stack_area, kRpcTaskSize);
struct k_thread rpc_thread_data;

chip::rpc::LightingService lighting_service;
chip::rpc::DeviceCommon device_common;

void RegisterServices(pw::rpc::Server & server)
{
    server.RegisterService(lighting_service);
    server.RegisterService(device_common);
}

} // namespace

k_tid_t Init()
{
    pw_sys_io_Init();
    k_tid_t tid = k_thread_create(&rpc_thread_data, rpc_stack_area, K_THREAD_STACK_SIZEOF(rpc_stack_area), RunRpcService, NULL,
                                  NULL, NULL, kRpcPriority, 0, K_NO_WAIT);
    return tid;
}

void RunRpcService(void *, void *, void *)
{
    Start(RegisterServices, &logger_mutex);
}

} // namespace rpc
} // namespace chip
