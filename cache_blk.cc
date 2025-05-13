/*
 * Copyright (c) 2012-2013, 2023-2024 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2020 Inria
 * Copyright (c) 2007 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "mem/cache/cache_blk.hh"

#include "base/cprintf.hh"

#include "base/trace.hh"

#include "debug/Cache.hh"

namespace gem5
{

const char*
CacheBlk::getStateString() const
{
    if(!isValid()) return "I";
    if(isSet(CacheBlk::DirtyBit)) return "M";
    if(isSet(CacheBlk::WritableBit)) return "E";
    return "S";
}

// void
// CacheBlk::insert(const KeyType &tag,
//                  const int src_requestor_ID, const uint32_t task_ID,
//                  const uint64_t partition_id)
// {
//     // Make sure that the block has been properly invalidated
//     assert(!isValid());

//     insert(tag);

//     // Set source requestor ID
//     setSrcRequestorId(src_requestor_ID);

//     // Set task ID
//     setTaskId(task_ID);

//     // Set partition ID
//     setPartitionId(partition_id);

//     // Set insertion tick as current tick
//     setTickInserted();

//     // Insertion counts as a reference to the block
//     increaseRefCount();

//     if (some_condition) {
//         if (!isSet(DirtyBit)) {
//             DPRINTF(Cache, "Block %#x set Dirty (state: %s -> M)\n",
//                     getTag(), getStateString());
//             setBit(DirtyBit);
//         }
//     }

// }

void
CacheBlk::logStateChange(const char* from, const char* to, const char* reason)
{
    if(strcmp(from, to) != 0){
        DPRINTF(Cache, "Block %#x state change: %s -> %s (%s)\n",
                getTag(), from, to, reason);
    }
}

void
CacheBlk::setDirty()
{
    const char* oldState = getStateString();
    if(!isSet(DirtyBit)){
        setCoherenceBits(DirtyBit);
        const char* newState = getStateString();
        logStateChange(oldState, newState, "setDirty");
    }
}

void
CacheBlk::clearDirty()
{
    const char* oldState = getStateString();
    if(isSet(DirtyBit)){
        clearCoherenceBits(DirtyBit);
        const char* newState = getStateString();
        logStateChange(oldState, newState, "clearDirty");
    }
}

void
CacheBlk::insert(const KeyType &tag,
                 const int src_requestor_ID, const uint32_t task_ID,
                 const uint64_t partition_id)
{
    const char* oldState = getStateString();
    assert(!isValid());

    TaggedEntry::insert(tag);

    setSrcRequestorId(src_requestor_ID);
    setTaskId(task_ID);
    setPartitionId(partition_id);
    setTickInserted();
    setRefCount(1); // insertion counts as a reference

    const char* newState = getStateString();
    logStateChange(oldState, newState, "insert");
}

void
CacheBlk::invalidate()
{
    const char* oldState = getStateString();

    TaggedEntry::invalidate();
    clearPrefetched();
    clearCoherenceBits(AllBits);
    setTaskId(context_switch_task_id::Unknown);
    setPartitionId(std::numeric_limits<uint64_t>::max());
    setWhenReady(MaxTick);
    setRefCount(0);
    setSrcRequestorId(Request::invldRequestorId);
    lockList.clear();

    const char* newState = getStateString();
    logStateChange(oldState, newState, "invalidate");
}

void
CacheBlkPrintWrapper::print(std::ostream &os, int verbosity,
                            const std::string &prefix) const
{
    ccprintf(os, "%sblk %c%c%c%c\n", prefix,
             blk->isValid()    ? 'V' : '-',
             blk->isSet(CacheBlk::WritableBit) ? 'E' : '-',
             blk->isSet(CacheBlk::DirtyBit)    ? 'M' : '-',
             blk->isSecure()   ? 'S' : '-');
}

} // namespace gem5