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

/**
 *    @file
 *      This file defines object for a CHIP IM Invoke Command Handler
 *
 */

#include "CommandHandler.h"
#include "Command.h"
#include "CommandSender.h"
#include "InteractionModelEngine.h"

#include <protocols/secure_channel/Constants.h>

using GeneralStatusCode = chip::Protocols::SecureChannel::GeneralStatusCode;

namespace chip {
namespace app {
void CommandHandler::OnMessageReceived(Messaging::ExchangeContext * ec, const PacketHeader & packetHeader,
                                       const PayloadHeader & payloadHeader, System::PacketBufferHandle payload)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    System::PacketBufferHandle response;

    // NOTE: we already know this is an InvokeCommand Request message because we explicitly registered with the
    // Exchange Manager for unsolicited InvokeCommand Requests.

    mpExchangeCtx = ec;

    err = ProcessCommandMessage(std::move(payload), CommandRoleId::HandlerId);
    SuccessOrExit(err);

    SendCommandResponse();

exit:
    ChipLogFunctError(err);
}

CHIP_ERROR CommandHandler::SendCommandResponse()
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    err = FinalizeCommandsMessage();
    SuccessOrExit(err);

    VerifyOrExit(mpExchangeCtx != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    err = mpExchangeCtx->SendMessage(Protocols::InteractionModel::MsgType::InvokeCommandResponse, std::move(mCommandMessageBuf),
                                     Messaging::SendFlags(Messaging::SendMessageFlags::kNone));
    SuccessOrExit(err);

    MoveToState(CommandState::Sending);

exit:
    Shutdown();
    ChipLogFunctError(err);
    return err;
}

CHIP_ERROR CommandHandler::ProcessCommandDataElement(CommandDataElement::Parser & aCommandElement)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    CommandPath::Parser commandPath;
    chip::TLV::TLVReader commandDataReader;
    chip::ClusterId clusterId;
    chip::CommandId commandId;
    chip::EndpointId endpointId;

    ReturnErrorOnFailure(aCommandElement.GetCommandPath(&commandPath));
    ReturnErrorOnFailure(commandPath.GetClusterId(&clusterId));
    ReturnErrorOnFailure(commandPath.GetCommandId(&commandId));
    ReturnErrorOnFailure(commandPath.GetEndpointId(&endpointId));

    err = aCommandElement.GetData(&commandDataReader);
    if (CHIP_END_OF_TLV == err)
    {
        // Empty Command, Add status code in invoke command response, notify cluster handler to hand it further.
        err = CHIP_NO_ERROR;
        ChipLogDetail(DataManagement, "Add Status code for empty command, cluster Id is %d", clusterId);
        AddStatusCode(GeneralStatusCode::kSuccess, Protocols::SecureChannel::Id, Protocols::SecureChannel::kProtocolCodeSuccess);
    }
    else if (CHIP_NO_ERROR == err)
    {
        DispatchSingleClusterCommand(clusterId, commandId, endpointId, commandDataReader, this);
    }

    return err;
}
} // namespace app
} // namespace chip
