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

#include "LightingManager.h"
#include "pigweed_lighting.rpc.pb.h"
#include "pw_hdlc/rpc_channel.h"
#include "pw_hdlc/rpc_packets.h"
#include "pw_rpc/server.h"
#include "pw_rpc_system_server/rpc_server.h"
#include "pw_stream/sys_io_stream.h"

#include <platform/CHIPDeviceError.h>

#include <thread>

namespace chip {
namespace rpc {

class LightingService final : public generated::LightingService<LightingService>
{
public:
    pw::Status ButtonEvent(ServerContext & ctx, const chip_rpc_Button & request, chip_rpc_Empty & response)
    {
        return pw::Status::Unimplemented();
    }

    pw::Status SetState(ServerContext &, const chip_rpc_LightEnabled & request, chip_rpc_Empty & response)
    {
        LightingMgr().InitiateAction(request.enabled ? LightingManager::ON_ACTION : LightingManager::OFF_ACTION);
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
        return pw::Status::Unimplemented();
    }
    pw::Status Reset(ServerContext & ctx, const chip_rpc_Empty & request, chip_rpc_Empty & response)
    {
        return pw::Status::Unimplemented();
    }
    pw::Status TriggerOta(ServerContext & ctx, const chip_rpc_Empty & request, chip_rpc_Empty & response)
    {
        return pw::Status::Unimplemented();
    }
    pw::Status DeviceInfo(ServerContext &, const chip_rpc_Empty & request, chip_rpc_DeviceDetails & response)
    {
        snprintf(response.fw_version, sizeof(response.fw_version), "0.0.0");
        snprintf(response.vendor_id, sizeof(response.vendor_id), "Linux");
        snprintf(response.product_id, sizeof(response.product_id), "LightingApp");
        snprintf(response.serial_number, sizeof(response.serial_number), "1234567890");
        return pw::OkStatus();
    }
};

namespace {
using std::byte;

chip::rpc::LightingService lighting_service;
chip::rpc::DeviceCommon device_common;

void RegisterServices(pw::rpc::Server & server)
{
    server.RegisterService(lighting_service);
    server.RegisterService(device_common);
}

} // namespace

void RunRpcService()
{
    pw::rpc::system_server::Init();
    RegisterServices(pw::rpc::system_server::Server());
    pw::rpc::system_server::Start();
}

int Init()
{
    int err = 0;
    std::thread rpc_service(RunRpcService);
    rpc_service.detach();
    return err;
}

} // namespace rpc
} // namespace chip
