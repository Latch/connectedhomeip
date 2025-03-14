/*
 *   Copyright (c) 2022 Project CHIP Authors
 *   All rights reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#pragma once

#include "../common/CHIPCommandBridge.h"

class OpenCommissioningWindowCommand : public CHIPCommandBridge
{
public:
    OpenCommissioningWindowCommand() : CHIPCommandBridge("open-commissioning-window")
    {
        AddArgument("node-id", 0, UINT64_MAX, &mNodeId, "Node to open a commissionging window on.");
        AddArgument("option", 0, 1, &mCommissioningWindowOption,
                    "1 to use Enhanced Commissioning Method.\n  0 to use Basic Commissioning Method.");
        AddArgument("window-timeout", 0, UINT16_MAX, &mCommissioningWindowTimeoutMs,
                    "Time, in seconds, before the commissioning window closes.");
        AddArgument("iteration", chip::kSpake2p_Min_PBKDF_Iterations, chip::kSpake2p_Max_PBKDF_Iterations, &mIteration,
                    "Number of PBKDF iterations to use to derive the verifier.  Ignored if 'option' is 0.");
        AddArgument("discriminator", 0, 4096, &mDiscriminator, "Discriminator to use for advertising.  Ignored if 'option' is 0.");
        AddArgument("timeout", 0, UINT16_MAX, &mTimeout, "Time, in seconds, before this command is considered to have timed out.");
    }

protected:
    /////////// CHIPCommandBridge Interface /////////
    CHIP_ERROR RunCommand() override;
    // We issue multiple data model operations for this command, and the default
    // timeout for those is 10 seconds, so default to 20 seconds.
    chip::System::Clock::Timeout GetWaitDuration() const override { return chip::System::Clock::Seconds16(mTimeout.ValueOr(20)); }

    chip::NodeId mNodeId;
    uint8_t mCommissioningWindowOption;
    uint16_t mCommissioningWindowTimeoutMs;
    uint32_t mIteration;
    uint16_t mDiscriminator;

    chip::Optional<uint16_t> mTimeout;

    // TODO: We should not need these members if we got sane callbacks from the
    // framework when the commissioning window is open.
    dispatch_queue_t mWorkQueue;
    dispatch_source_t mTimer;
};
