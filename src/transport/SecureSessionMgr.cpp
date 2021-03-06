/*
 *
 *    Copyright (c) 2020-2021 Project CHIP Authors
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 *      This file implements the CHIP Connection object that maintains a UDP connection.
 *      TODO This class should be extended to support TCP as well...
 *
 */

#include "SecureSessionMgr.h"

#include <inttypes.h>
#include <string.h>

#include <core/CHIPKeyIds.h>
#include <platform/CHIPDeviceLayer.h>
#include <support/CodeUtils.h>
#include <support/SafeInt.h>
#include <support/logging/CHIPLogging.h>
#include <transport/AdminPairingTable.h>
#include <transport/PASESession.h>
#include <transport/RendezvousSession.h>
#include <transport/SecureMessageCodec.h>
#include <transport/SecureSessionMgr.h>
#include <transport/TransportMgr.h>

#include <inttypes.h>

namespace chip {

using System::PacketBufferHandle;
using Transport::PeerAddress;
using Transport::PeerConnectionState;

// Maximum length of application data that can be encrypted as one block.
// The limit is derived from IPv6 MTU (1280 bytes) - expected header overheads.
// This limit would need additional reviews once we have formalized Secure Transport header.
//
// TODO: this should be checked within the transport message sending instead of the session management layer.
static const size_t kMax_SecureSDU_Length = 1024;

SecureSessionMgr::SecureSessionMgr() : mState(State::kNotReady) {}

SecureSessionMgr::~SecureSessionMgr()
{
    CancelExpiryTimer();
}

CHIP_ERROR SecureSessionMgr::Init(NodeId localNodeId, System::Layer * systemLayer, TransportMgrBase * transportMgr,
                                  Transport::AdminPairingTable * admins)
{
    VerifyOrReturnError(mState == State::kNotReady, CHIP_ERROR_INCORRECT_STATE);
    VerifyOrReturnError(transportMgr != nullptr, CHIP_ERROR_INVALID_ARGUMENT);

    mState        = State::kInitialized;
    mLocalNodeId  = localNodeId;
    mSystemLayer  = systemLayer;
    mTransportMgr = transportMgr;
    mAdmins       = admins;

    ChipLogProgress(Inet, "local node id is 0x%08" PRIx32 "%08" PRIx32, static_cast<uint32_t>(mLocalNodeId >> 32),
                    static_cast<uint32_t>(mLocalNodeId));

    ScheduleExpiryTimer();

    mTransportMgr->SetSecureSessionMgr(this);

    return CHIP_NO_ERROR;
}

void SecureSessionMgr::Shutdown()
{
    CancelExpiryTimer();

    mState        = State::kNotReady;
    mLocalNodeId  = kUndefinedNodeId;
    mSystemLayer  = nullptr;
    mTransportMgr = nullptr;
    mAdmins       = nullptr;
    mCB           = nullptr;
}

Transport::Type SecureSessionMgr::GetTransportType(NodeId peerNodeId)
{
    PeerConnectionState * state = mPeerConnections.FindPeerConnectionState(peerNodeId, nullptr);

    if (state)
    {
        return state->GetPeerAddress().GetTransportType();
    }

    return Transport::Type::kUndefined;
}

CHIP_ERROR SecureSessionMgr::SendMessage(SecureSessionHandle session, System::PacketBufferHandle && msgBuf)
{
    PayloadHeader unusedPayloadHeader;
    return SendMessage(session, unusedPayloadHeader, std::move(msgBuf));
}

CHIP_ERROR SecureSessionMgr::SendMessage(SecureSessionHandle session, PayloadHeader & payloadHeader,
                                         System::PacketBufferHandle && msgBuf, EncryptedPacketBufferHandle * bufferRetainSlot)
{
    PacketHeader unusedPacketHeader;
    return SendMessage(session, payloadHeader, unusedPacketHeader, std::move(msgBuf), bufferRetainSlot,
                       EncryptionState::kPayloadIsUnencrypted);
}

CHIP_ERROR SecureSessionMgr::SendEncryptedMessage(SecureSessionHandle session, EncryptedPacketBufferHandle msgBuf,
                                                  EncryptedPacketBufferHandle * bufferRetainSlot)
{
    VerifyOrReturnError(!msgBuf.IsNull(), CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(!msgBuf->HasChainedBuffer(), CHIP_ERROR_INVALID_MESSAGE_LENGTH);
    VerifyOrReturnError(msgBuf->TotalLength() < kMax_SecureSDU_Length, CHIP_ERROR_INVALID_MESSAGE_LENGTH);

    // Advancing the start to encrypted header, since the transport will attach the packet header on top of it
    PacketHeader packetHeader;
    ReturnErrorOnFailure(packetHeader.DecodeAndConsume(msgBuf));

    PayloadHeader payloadHeader;
    return SendMessage(session, payloadHeader, packetHeader, std::move(msgBuf), bufferRetainSlot,
                       EncryptionState::kPayloadIsEncrypted);
}

CHIP_ERROR SecureSessionMgr::SendMessage(SecureSessionHandle session, PayloadHeader & payloadHeader, PacketHeader & packetHeader,
                                         System::PacketBufferHandle msgBuf, EncryptedPacketBufferHandle * bufferRetainSlot,
                                         EncryptionState encryptionState)
{
    CHIP_ERROR err              = CHIP_NO_ERROR;
    PeerConnectionState * state = nullptr;
    uint8_t * msgStart          = nullptr;
    uint16_t msgLen             = 0;
    uint16_t headerSize         = 0;
    NodeId localNodeId          = mLocalNodeId;

    Transport::AdminPairingInfo * admin = nullptr;

    // Hold the reference to encrypted message in stack variable.
    // In case of any failures, the reference is not returned, and this stack variable
    // will automatically free the reference on returning from the function.
    EncryptedPacketBufferHandle encryptedMsg;

    VerifyOrExit(mState == State::kInitialized, err = CHIP_ERROR_INCORRECT_STATE);

    VerifyOrExit(!msgBuf.IsNull(), err = CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(!msgBuf->HasChainedBuffer(), err = CHIP_ERROR_INVALID_MESSAGE_LENGTH);
    VerifyOrExit(msgBuf->TotalLength() < kMax_SecureSDU_Length, err = CHIP_ERROR_INVALID_MESSAGE_LENGTH);

    // Find an active connection to the specified peer node
    state = GetPeerConnectionState(session);
    VerifyOrExit(state != nullptr, err = CHIP_ERROR_NOT_CONNECTED);

    // This marks any connection where we send data to as 'active'
    mPeerConnections.MarkConnectionActive(state);
    admin = mAdmins->FindAdmin(state->GetAdminId());
    VerifyOrExit(admin != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    localNodeId = admin->GetNodeId();

    if (payloadHeader.HasMessageType(Protocols::SecureChannel::MsgType::MsgCounterSyncReq) ||
        payloadHeader.HasMessageType(Protocols::SecureChannel::MsgType::MsgCounterSyncRsp))
    {
        packetHeader.SetSecureSessionControlMsg(true);
    }

    if (encryptionState == EncryptionState::kPayloadIsUnencrypted)
    {
        err = SecureMessageCodec::Encode(localNodeId, state, payloadHeader, packetHeader, msgBuf);
        SuccessOrExit(err);
    }

    // The start of buffer points to the beginning of the encrypted header, and the length of buffer
    // contains both the encrypted header and encrypted data.
    // Locally store the start and length of the retained buffer after accounting for the size of packet header.
    headerSize = packetHeader.EncodeSizeBytes();

    msgStart = static_cast<uint8_t *>(msgBuf->Start() - headerSize);
    msgLen   = static_cast<uint16_t>(msgBuf->DataLength() + headerSize);

    // Retain the packet buffer in case it's needed for retransmissions.
    if (bufferRetainSlot != nullptr)
    {
        encryptedMsg        = msgBuf.Retain();
        encryptedMsg.mMsgId = packetHeader.GetMessageId();
    }

    ChipLogProgress(Inet, "Sending msg from 0x%08" PRIx32 "%08" PRIx32 "to 0x%08" PRIx32 "%08" PRIx32,
                    static_cast<uint32_t>(localNodeId >> 32), static_cast<uint32_t>(localNodeId),
                    static_cast<uint32_t>(state->GetPeerNodeId() >> 32), static_cast<uint32_t>(state->GetPeerNodeId()));

    if (state->GetTransport() != nullptr)
    {
        ChipLogProgress(Inet, "Sending secure msg on connection specific transport");
        err = state->GetTransport()->SendMessage(packetHeader, state->GetPeerAddress(), std::move(msgBuf));
    }
    else
    {
        ChipLogProgress(Inet, "Sending secure msg on generic transport");
        err = mTransportMgr->SendMessage(packetHeader, state->GetPeerAddress(), std::move(msgBuf));
    }
    ChipLogProgress(Inet, "Secure msg send status %d", err);
    SuccessOrExit(err);

    if (bufferRetainSlot != nullptr)
    {
        // Rewind the start and len of the buffer back to pre-send state for following possible retransmition.
        encryptedMsg->SetStart(msgStart);
        encryptedMsg->SetDataLength(msgLen);

        (*bufferRetainSlot) = std::move(encryptedMsg);
    }

exit:
    if (!msgBuf.IsNull())
    {
        const char * errStr = ErrorStr(err);
        if (state == nullptr)
        {
            ChipLogError(Inet, "Secure transport could not find a valid PeerConnection: %s", errStr);
        }
    }

    return err;
}

CHIP_ERROR SecureSessionMgr::NewPairing(const Optional<Transport::PeerAddress> & peerAddr, NodeId peerNodeId, PASESession * pairing,
                                        PairingDirection direction, Transport::AdminId admin, Transport::Base * transport)
{
    uint16_t peerKeyId          = pairing->GetPeerKeyId();
    uint16_t localKeyId         = pairing->GetLocalKeyId();
    PeerConnectionState * state = mPeerConnections.FindPeerConnectionState(Optional<NodeId>::Value(peerNodeId), peerKeyId, nullptr);

    // Find any existing connection with the same node and key ID
    if (state && (state->GetAdminId() == Transport::kUndefinedAdminId || state->GetAdminId() == admin))
    {
        mPeerConnections.MarkConnectionExpired(
            state, [this](const Transport::PeerConnectionState & state1) { HandleConnectionExpired(state1); });
    }

    ChipLogDetail(Inet, "New pairing for device 0x%08" PRIx32 "%08" PRIx32 ", key %d!!", static_cast<uint32_t>(peerNodeId >> 32),
                  static_cast<uint32_t>(peerNodeId), peerKeyId);
    state = nullptr;
    ReturnErrorOnFailure(
        mPeerConnections.CreateNewPeerConnectionState(Optional<NodeId>::Value(peerNodeId), peerKeyId, localKeyId, &state));

    state->SetAdminId(admin);
    state->SetTransport(transport);

    if (peerAddr.HasValue() && peerAddr.Value().GetIPAddress() != Inet::IPAddress::Any)
    {
        state->SetPeerAddress(peerAddr.Value());
    }
    else if (peerAddr.HasValue() &&
             (peerAddr.Value().GetTransportType() == Transport::Type::kTcp ||
              peerAddr.Value().GetTransportType() == Transport::Type::kUdp))
    {
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    if (state != nullptr)
    {
        switch (direction)
        {
        case PairingDirection::kInitiator:
            ReturnErrorOnFailure(pairing->DeriveSecureSession(reinterpret_cast<const uint8_t *>(kSpake2pI2RSessionInfo),
                                                              strlen(kSpake2pI2RSessionInfo), state->GetSenderSecureSession()));

            ReturnErrorOnFailure(pairing->DeriveSecureSession(reinterpret_cast<const uint8_t *>(kSpake2pR2ISessionInfo),
                                                              strlen(kSpake2pR2ISessionInfo), state->GetReceiverSecureSession()));

            break;
        case PairingDirection::kResponder:
            ReturnErrorOnFailure(pairing->DeriveSecureSession(reinterpret_cast<const uint8_t *>(kSpake2pR2ISessionInfo),
                                                              strlen(kSpake2pR2ISessionInfo), state->GetSenderSecureSession()));
            ReturnErrorOnFailure(pairing->DeriveSecureSession(reinterpret_cast<const uint8_t *>(kSpake2pI2RSessionInfo),
                                                              strlen(kSpake2pI2RSessionInfo), state->GetReceiverSecureSession()));

            break;
        default:
            return CHIP_ERROR_INVALID_ARGUMENT;
        };

        if (mCB != nullptr)
        {
            mCB->OnNewConnection({ state->GetPeerNodeId(), state->GetPeerKeyID(), admin }, this);
        }
    }

    return CHIP_NO_ERROR;
}

void SecureSessionMgr::ScheduleExpiryTimer()
{
    CHIP_ERROR err =
        mSystemLayer->StartTimer(CHIP_PEER_CONNECTION_TIMEOUT_CHECK_FREQUENCY_MS, SecureSessionMgr::ExpiryTimerCallback, this);

    VerifyOrDie(err == CHIP_NO_ERROR);
}

void SecureSessionMgr::CancelExpiryTimer()
{
    if (mSystemLayer != nullptr)
    {
        mSystemLayer->CancelTimer(SecureSessionMgr::ExpiryTimerCallback, this);
    }
}

void SecureSessionMgr::OnMessageReceived(const PacketHeader & packetHeader, const PeerAddress & peerAddress,
                                         System::PacketBufferHandle msg)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    PeerConnectionState * state = mPeerConnections.FindPeerConnectionState(packetHeader.GetEncryptionKeyID(), nullptr);

    PacketBufferHandle origMsg;
    PayloadHeader payloadHeader;

    bool peerGroupMsgIdNotSynchronized  = false;
    Transport::AdminPairingInfo * admin = nullptr;

    VerifyOrExit(!msg.IsNull(), ChipLogError(Inet, "Secure transport received NULL packet, discarding"));

    if (state == nullptr)
    {
        ChipLogError(Inet, "Data received on an unknown connection (%d). Dropping it!!", packetHeader.GetEncryptionKeyID());
        ExitNow(err = CHIP_ERROR_KEY_NOT_FOUND_FROM_PEER);
    }

    admin = mAdmins->FindAdmin(state->GetAdminId());
    VerifyOrExit(admin != nullptr, ChipLogError(Inet, "Secure transport received packet for unknown admin pairing, discarding"));
    if (packetHeader.GetDestinationNodeId().HasValue())
    {
        VerifyOrExit(
            admin->GetNodeId() == packetHeader.GetDestinationNodeId().Value(),
            ChipLogError(Inet, "Secure transport received message, but destination node ID doesn't match our node ID, discarding"));
    }
    mPeerConnections.MarkConnectionActive(state);

    // Decode the message
    VerifyOrReturn(CHIP_NO_ERROR == SecureMessageCodec::Decode(state, payloadHeader, packetHeader, msg));

    if (state->GetPeerNodeId() == kUndefinedNodeId && packetHeader.GetSourceNodeId().HasValue())
    {
        state->SetPeerNodeId(packetHeader.GetSourceNodeId().Value());
    }

    // TODO: once mDNS address resolution is available reconsider if this is required
    // This updates the peer address once a packet is received from a new address
    // and serves as a way to auto-detect peer changing IPs.
    if (state->GetPeerAddress() != peerAddress)
    {
        state->SetPeerAddress(peerAddress);
    }

    if (!state->IsPeerMsgCounterSynced())
    {
        // For all control messages, the first authenticated message counter from an unsynchronized peer is trusted
        // and used to seed subsequent message counter based replay protection.
        if (packetHeader.IsSecureSessionControlMsg())
        {
            state->SetPeerMessageIndex(packetHeader.GetMessageId());
        }

        // For all group messages, Set flag if peer group key message counter is not synchronized.
        if (ChipKeyId::IsAppGroupKey(packetHeader.GetEncryptionKeyID()))
        {
            peerGroupMsgIdNotSynchronized = true;
        }
    }

    if (mCB != nullptr)
    {
        SecureSessionHandle session(state->GetPeerNodeId(), state->GetPeerKeyID(), state->GetAdminId());

        session.SetPeerGroupMsgIdNotSynchronized(peerGroupMsgIdNotSynchronized);

        mCB->OnMessageReceived(packetHeader, payloadHeader, session, std::move(msg), this);
    }

exit:
    if (err != CHIP_NO_ERROR && mCB != nullptr)
    {
        mCB->OnReceiveError(err, peerAddress, this);
    }
}

void SecureSessionMgr::HandleConnectionExpired(const Transport::PeerConnectionState & state)
{
    char addr[Transport::PeerAddress::kMaxToStringSize];
    state.GetPeerAddress().ToString(addr);

    ChipLogDetail(Inet, "Connection from '%s' expired", addr);

    if (mCB != nullptr)
    {
        mCB->OnConnectionExpired({ state.GetPeerNodeId(), state.GetPeerKeyID(), state.GetAdminId() }, this);
    }

    mTransportMgr->Disconnect(state.GetPeerAddress());
}

void SecureSessionMgr::ExpiryTimerCallback(System::Layer * layer, void * param, System::Error error)
{
    SecureSessionMgr * mgr = reinterpret_cast<SecureSessionMgr *>(param);
#if CHIP_CONFIG_SESSION_REKEYING
    // TODO(#2279): session expiration is currently disabled until rekeying is supported
    // the #ifdef should be removed after that.
    mgr->mPeerConnections.ExpireInactiveConnections(
        CHIP_PEER_CONNECTION_TIMEOUT_MS,
        [this](const Transport::PeerConnectionState & state1) { HandleConnectionExpired(state1); });
#endif
    mgr->ScheduleExpiryTimer(); // re-schedule the oneshot timer
}

PeerConnectionState * SecureSessionMgr::GetPeerConnectionState(SecureSessionHandle session)
{
    return mPeerConnections.FindPeerConnectionState(Optional<NodeId>::Value(session.mPeerNodeId), session.mPeerKeyId, nullptr);
}

} // namespace chip
