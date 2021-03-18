/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
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

// THIS FILE IS GENERATED BY ZAP

#include <cinttypes>
#include <cstdint>

#include "af-structs.h"
#include "app/util/util.h"
#include "call-command-handler.h"
#include "callback.h"
#include "cluster-id.h"
#include "command-id.h"

#include <app/InteractionModelEngine.h>

// Currently we need some work to keep compatible with ember lib.
#include <util/ember-compatibility-functions.h>

namespace chip {
namespace app {

// Cluster specific command parsing

namespace clusters {

namespace NetworkCommissioning {

void DispatchServerCommand(app::Command * command, CommandId commandId, EndpointId endpointId, TLV::TLVReader & dataTlv)
{
    {
        switch (commandId)
        {
        case ZCL_ADD_THREAD_NETWORK_COMMAND_ID: {
            CHIP_ERROR TLVError = CHIP_NO_ERROR;
            const uint8_t * operationalDataset;
            uint64_t breadcrumb;
            uint32_t timeoutMs;

            while ((TLVError = dataTlv.Next()) == CHIP_NO_ERROR)
            {
                switch (TLV::TagNumFromTag(dataTlv.GetTag()))
                {
                case 0:
                    TLVError = dataTlv.GetDataPtr(operationalDataset);
                    break;
                case 1:
                    TLVError = dataTlv.Get(breadcrumb);
                    break;
                case 2:
                    TLVError = dataTlv.Get(timeoutMs);
                    break;
                default:
                    // Unsupported tag, ignore it.
                    ChipLogProgress(Zcl, "Unknown TLV tag during processing.");
                    break;
                }
                if (TLVError != CHIP_NO_ERROR)
                {
                    ChipLogProgress(Zcl, "Failed to decode TLV data with tag %" PRIx32 ": %" PRId32,
                                    TLV::TagNumFromTag(dataTlv.GetTag()), TLVError);
                }
            }
            // TODO(#5098) We should pass the Command Object and EndpointId to the cluster callbacks.
            emberAfNetworkCommissioningClusterAddThreadNetworkCallback(const_cast<uint8_t *>(operationalDataset), breadcrumb,
                                                                       timeoutMs);
            break;
        }
        case ZCL_ADD_WI_FI_NETWORK_COMMAND_ID: {
            CHIP_ERROR TLVError = CHIP_NO_ERROR;
            const uint8_t * ssid;
            const uint8_t * credentials;
            uint64_t breadcrumb;
            uint32_t timeoutMs;

            while ((TLVError = dataTlv.Next()) == CHIP_NO_ERROR)
            {
                switch (TLV::TagNumFromTag(dataTlv.GetTag()))
                {
                case 0:
                    TLVError = dataTlv.GetDataPtr(ssid);
                    break;
                case 1:
                    TLVError = dataTlv.GetDataPtr(credentials);
                    break;
                case 2:
                    TLVError = dataTlv.Get(breadcrumb);
                    break;
                case 3:
                    TLVError = dataTlv.Get(timeoutMs);
                    break;
                default:
                    // Unsupported tag, ignore it.
                    ChipLogProgress(Zcl, "Unknown TLV tag during processing.");
                    break;
                }
                if (TLVError != CHIP_NO_ERROR)
                {
                    ChipLogProgress(Zcl, "Failed to decode TLV data with tag %" PRIx32 ": %" PRId32,
                                    TLV::TagNumFromTag(dataTlv.GetTag()), TLVError);
                }
            }
            // TODO(#5098) We should pass the Command Object and EndpointId to the cluster callbacks.
            emberAfNetworkCommissioningClusterAddWiFiNetworkCallback(const_cast<uint8_t *>(ssid),
                                                                     const_cast<uint8_t *>(credentials), breadcrumb, timeoutMs);
            break;
        }
        case ZCL_DISABLE_NETWORK_COMMAND_ID: {
            CHIP_ERROR TLVError = CHIP_NO_ERROR;
            const uint8_t * networkID;
            uint64_t breadcrumb;
            uint32_t timeoutMs;

            while ((TLVError = dataTlv.Next()) == CHIP_NO_ERROR)
            {
                switch (TLV::TagNumFromTag(dataTlv.GetTag()))
                {
                case 0:
                    TLVError = dataTlv.GetDataPtr(networkID);
                    break;
                case 1:
                    TLVError = dataTlv.Get(breadcrumb);
                    break;
                case 2:
                    TLVError = dataTlv.Get(timeoutMs);
                    break;
                default:
                    // Unsupported tag, ignore it.
                    ChipLogProgress(Zcl, "Unknown TLV tag during processing.");
                    break;
                }
                if (TLVError != CHIP_NO_ERROR)
                {
                    ChipLogProgress(Zcl, "Failed to decode TLV data with tag %" PRIx32 ": %" PRId32,
                                    TLV::TagNumFromTag(dataTlv.GetTag()), TLVError);
                }
            }
            // TODO(#5098) We should pass the Command Object and EndpointId to the cluster callbacks.
            emberAfNetworkCommissioningClusterDisableNetworkCallback(const_cast<uint8_t *>(networkID), breadcrumb, timeoutMs);
            break;
        }
        case ZCL_ENABLE_NETWORK_COMMAND_ID: {
            CHIP_ERROR TLVError = CHIP_NO_ERROR;
            const uint8_t * networkID;
            uint64_t breadcrumb;
            uint32_t timeoutMs;

            while ((TLVError = dataTlv.Next()) == CHIP_NO_ERROR)
            {
                switch (TLV::TagNumFromTag(dataTlv.GetTag()))
                {
                case 0:
                    TLVError = dataTlv.GetDataPtr(networkID);
                    break;
                case 1:
                    TLVError = dataTlv.Get(breadcrumb);
                    break;
                case 2:
                    TLVError = dataTlv.Get(timeoutMs);
                    break;
                default:
                    // Unsupported tag, ignore it.
                    ChipLogProgress(Zcl, "Unknown TLV tag during processing.");
                    break;
                }
                if (TLVError != CHIP_NO_ERROR)
                {
                    ChipLogProgress(Zcl, "Failed to decode TLV data with tag %" PRIx32 ": %" PRId32,
                                    TLV::TagNumFromTag(dataTlv.GetTag()), TLVError);
                }
            }
            // TODO(#5098) We should pass the Command Object and EndpointId to the cluster callbacks.
            emberAfNetworkCommissioningClusterEnableNetworkCallback(const_cast<uint8_t *>(networkID), breadcrumb, timeoutMs);
            break;
        }
        case ZCL_GET_LAST_NETWORK_COMMISSIONING_RESULT_COMMAND_ID: {
            CHIP_ERROR TLVError = CHIP_NO_ERROR;
            uint32_t timeoutMs;

            while ((TLVError = dataTlv.Next()) == CHIP_NO_ERROR)
            {
                switch (TLV::TagNumFromTag(dataTlv.GetTag()))
                {
                case 0:
                    TLVError = dataTlv.Get(timeoutMs);
                    break;
                default:
                    // Unsupported tag, ignore it.
                    ChipLogProgress(Zcl, "Unknown TLV tag during processing.");
                    break;
                }
                if (TLVError != CHIP_NO_ERROR)
                {
                    ChipLogProgress(Zcl, "Failed to decode TLV data with tag %" PRIx32 ": %" PRId32,
                                    TLV::TagNumFromTag(dataTlv.GetTag()), TLVError);
                }
            }
            // TODO(#5098) We should pass the Command Object and EndpointId to the cluster callbacks.
            emberAfNetworkCommissioningClusterGetLastNetworkCommissioningResultCallback(timeoutMs);
            break;
        }
        case ZCL_REMOVE_NETWORK_COMMAND_ID: {
            CHIP_ERROR TLVError = CHIP_NO_ERROR;
            const uint8_t * NetworkID;
            uint64_t Breadcrumb;
            uint32_t TimeoutMs;

            while ((TLVError = dataTlv.Next()) == CHIP_NO_ERROR)
            {
                switch (TLV::TagNumFromTag(dataTlv.GetTag()))
                {
                case 0:
                    TLVError = dataTlv.GetDataPtr(NetworkID);
                    break;
                case 1:
                    TLVError = dataTlv.Get(Breadcrumb);
                    break;
                case 2:
                    TLVError = dataTlv.Get(TimeoutMs);
                    break;
                default:
                    // Unsupported tag, ignore it.
                    ChipLogProgress(Zcl, "Unknown TLV tag during processing.");
                    break;
                }
                if (TLVError != CHIP_NO_ERROR)
                {
                    ChipLogProgress(Zcl, "Failed to decode TLV data with tag %" PRIx32 ": %" PRId32,
                                    TLV::TagNumFromTag(dataTlv.GetTag()), TLVError);
                }
            }
            // TODO(#5098) We should pass the Command Object and EndpointId to the cluster callbacks.
            emberAfNetworkCommissioningClusterRemoveNetworkCallback(const_cast<uint8_t *>(NetworkID), Breadcrumb, TimeoutMs);
            break;
        }
        case ZCL_SCAN_NETWORKS_COMMAND_ID: {
            CHIP_ERROR TLVError = CHIP_NO_ERROR;
            const uint8_t * ssid;
            uint64_t breadcrumb;
            uint32_t timeoutMs;

            while ((TLVError = dataTlv.Next()) == CHIP_NO_ERROR)
            {
                switch (TLV::TagNumFromTag(dataTlv.GetTag()))
                {
                case 0:
                    TLVError = dataTlv.GetDataPtr(ssid);
                    break;
                case 1:
                    TLVError = dataTlv.Get(breadcrumb);
                    break;
                case 2:
                    TLVError = dataTlv.Get(timeoutMs);
                    break;
                default:
                    // Unsupported tag, ignore it.
                    ChipLogProgress(Zcl, "Unknown TLV tag during processing.");
                    break;
                }
                if (TLVError != CHIP_NO_ERROR)
                {
                    ChipLogProgress(Zcl, "Failed to decode TLV data with tag %" PRIx32 ": %" PRId32,
                                    TLV::TagNumFromTag(dataTlv.GetTag()), TLVError);
                }
            }
            // TODO(#5098) We should pass the Command Object and EndpointId to the cluster callbacks.
            emberAfNetworkCommissioningClusterScanNetworksCallback(const_cast<uint8_t *>(ssid), breadcrumb, timeoutMs);
            break;
        }
        case ZCL_UPDATE_THREAD_NETWORK_COMMAND_ID: {
            CHIP_ERROR TLVError = CHIP_NO_ERROR;
            const uint8_t * operationalDataset;
            uint64_t breadcrumb;
            uint32_t timeoutMs;

            while ((TLVError = dataTlv.Next()) == CHIP_NO_ERROR)
            {
                switch (TLV::TagNumFromTag(dataTlv.GetTag()))
                {
                case 0:
                    TLVError = dataTlv.GetDataPtr(operationalDataset);
                    break;
                case 1:
                    TLVError = dataTlv.Get(breadcrumb);
                    break;
                case 2:
                    TLVError = dataTlv.Get(timeoutMs);
                    break;
                default:
                    // Unsupported tag, ignore it.
                    ChipLogProgress(Zcl, "Unknown TLV tag during processing.");
                    break;
                }
                if (TLVError != CHIP_NO_ERROR)
                {
                    ChipLogProgress(Zcl, "Failed to decode TLV data with tag %" PRIx32 ": %" PRId32,
                                    TLV::TagNumFromTag(dataTlv.GetTag()), TLVError);
                }
            }
            // TODO(#5098) We should pass the Command Object and EndpointId to the cluster callbacks.
            emberAfNetworkCommissioningClusterUpdateThreadNetworkCallback(const_cast<uint8_t *>(operationalDataset), breadcrumb,
                                                                          timeoutMs);
            break;
        }
        case ZCL_UPDATE_WI_FI_NETWORK_COMMAND_ID: {
            CHIP_ERROR TLVError = CHIP_NO_ERROR;
            const uint8_t * ssid;
            const uint8_t * credentials;
            uint64_t breadcrumb;
            uint32_t timeoutMs;

            while ((TLVError = dataTlv.Next()) == CHIP_NO_ERROR)
            {
                switch (TLV::TagNumFromTag(dataTlv.GetTag()))
                {
                case 0:
                    TLVError = dataTlv.GetDataPtr(ssid);
                    break;
                case 1:
                    TLVError = dataTlv.GetDataPtr(credentials);
                    break;
                case 2:
                    TLVError = dataTlv.Get(breadcrumb);
                    break;
                case 3:
                    TLVError = dataTlv.Get(timeoutMs);
                    break;
                default:
                    // Unsupported tag, ignore it.
                    ChipLogProgress(Zcl, "Unknown TLV tag during processing.");
                    break;
                }
                if (TLVError != CHIP_NO_ERROR)
                {
                    ChipLogProgress(Zcl, "Failed to decode TLV data with tag %" PRIx32 ": %" PRId32,
                                    TLV::TagNumFromTag(dataTlv.GetTag()), TLVError);
                }
            }
            // TODO(#5098) We should pass the Command Object and EndpointId to the cluster callbacks.
            emberAfNetworkCommissioningClusterUpdateWiFiNetworkCallback(const_cast<uint8_t *>(ssid),
                                                                        const_cast<uint8_t *>(credentials), breadcrumb, timeoutMs);
            break;
        }
        default: {
            // Unrecognized command ID, error status will apply.
            // TODO: Encode response for command not found
            ChipLogError(Zcl, "Unknown command %" PRIx16 " for cluster %" PRIx16, commandId, ZCL_NETWORK_COMMISSIONING_CLUSTER_ID);
            break;
        }
        }
    }
}

} // namespace NetworkCommissioning

} // namespace clusters

void DispatchSingleClusterCommand(chip::ClusterId aClusterId, chip::CommandId aCommandId, chip::EndpointId aEndPointId,
                                  chip::TLV::TLVReader & aReader, Command * apCommandObj)
{
    ChipLogDetail(Zcl, "Received Cluster Command: Cluster=%" PRIx16 " Command=%" PRIx8 " Endpoint=%" PRIx8, aClusterId, aCommandId,
                  aEndPointId);
    Compatibility::SetupEmberAfObjects(apCommandObj, aClusterId, aCommandId, aEndPointId);
    switch (aClusterId)
    {
    case ZCL_NETWORK_COMMISSIONING_CLUSTER_ID:
        clusters::NetworkCommissioning::DispatchServerCommand(apCommandObj, aCommandId, aEndPointId, aReader);
        break;
    default:
        // Unrecognized cluster ID, error status will apply.
        // TODO: Encode response for Cluster not found
        ChipLogError(Zcl, "Unknown cluster %" PRIx16, aClusterId);
        break;
    }
    Compatibility::ResetEmberAfObjects();
}

} // namespace app
} // namespace chip
