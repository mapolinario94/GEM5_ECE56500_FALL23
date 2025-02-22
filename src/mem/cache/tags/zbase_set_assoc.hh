/*
 * Copyright (c) 2012-2014,2017 ARM Limited
 * All rights reserved.
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
 * Copyright (c) 2003-2005,2014 The Regents of The University of Michigan
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

/**
 * @file
 * Declaration of a base set associative tag store.
 */

#ifndef __MEM_CACHE_TAGS_ZBASE_SET_ASSOC_HH__
#define __MEM_CACHE_TAGS_ZBASE_SET_ASSOC_HH__

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "base/logging.hh"
#include "base/types.hh"
#include "mem/cache/base.hh"
#include "mem/cache/cache_blk.hh"
#include "mem/cache/replacement_policies/base.hh"
#include "mem/cache/replacement_policies/replaceable_entry.hh"
#include "mem/cache/tags/base.hh"
#include "mem/cache/tags/indexing_policies/base.hh"
#include "mem/packet.hh"
#include "params/ZBaseSetAssoc.hh"
#include "debug/Ztags.hh"
#include "sim/cur_tick.hh"

namespace gem5
{

/**
 * A basic cache tag store.
 * @sa  \ref gem5MemorySystem "gem5 Memory System"
 *
 * The BaseSetAssoc placement policy divides the cache into s sets of w
 * cache lines (ways).
 */
class ZBaseSetAssoc : public BaseTags
{
  protected:
    /** The allocatable associativity of the cache (alloc mask). */
    unsigned allocAssoc;

    /** The cache blocks. */
    std::vector<CacheBlk> blks;

    /** Whether tags and data are accessed sequentially. */
    const bool sequentialAccess;

    /** Replacement policy */
    replacement_policy::Base *replacementPolicy;

  public:
    /** Convenience typedef. */
     typedef ZBaseSetAssocParams Params;

    /**
     * Construct and initialize this tag store.
     */
    ZBaseSetAssoc(const Params &p);

    /**
     * Destructor
     */
    virtual ~ZBaseSetAssoc() {};

    /**
     * Initialize blocks as CacheBlk instances.
     */
    void tagsInit() override;

    /**
     * This function updates the tags when a block is invalidated. It also
     * updates the replacement data.
     *
     * @param blk The block to invalidate.
     */
    void invalidate(CacheBlk *blk) override;

    /**
     * Access block and update replacement data. May not succeed, in which case
     * nullptr is returned. This has all the implications of a cache access and
     * should only be used as such. Returns the tag lookup latency as a side
     * effect.
     *
     * @param pkt The packet holding the address to find.
     * @param lat The latency of the tag lookup.
     * @return Pointer to the cache block if found.
     */
    CacheBlk* accessBlock(const PacketPtr pkt, Cycles &lat) override
    {
        CacheBlk *blk = findBlock(pkt->getAddr(), pkt->isSecure());

        // Access all tags in parallel, hence one in each way.  The data side
        // either accesses all blocks in parallel, or one block sequentially on
        // a hit.  Sequential access with a miss doesn't access data.
        stats.tagAccesses += allocAssoc;
        if (sequentialAccess) {
            if (blk != nullptr) {
                stats.dataAccesses += 1;
            }
        } else {
            stats.dataAccesses += allocAssoc;
        }

        // If a cache hit
        if (blk != nullptr) {
            // Update number of references to accessed block
            blk->increaseRefCount();

            // Update replacement data of accessed block
            replacementPolicy->touch(blk->replacementData, pkt);
        }

        // The tag lookup latency is the same for a hit or a miss
        lat = lookupLatency;

        return blk;
    }

    /**
     * Find replacement victim based on address. The list of evicted blocks
     * only contains the victim.
     *
     * @param addr Address to find a victim for.
     * @param is_secure True if the target memory space is secure.
     * @param size Size, in bits, of new block to allocate.
     * @param evict_blks Cache blocks to be evicted.
     * @return Cache block to be replaced.
     */
    CacheBlk* findVictim(Addr addr, const bool is_secure,
                         const std::size_t size,
                         std::vector<CacheBlk*>& evict_blks) override
    {
        DPRINTF(Ztags, "Before calling getPossibleEntries() \n");
        // Get possible entries to be victimized
        const std::vector<ReplaceableEntry*> entries =
            indexingPolicy->getPossibleEntries(addr);

        // DPRINTF(ZLRUTest, "Before calling getPossibleEntriesTree() \n");
        // const std::vector<ReplaceableEntry*> entries_tree =
        //     indexingPolicy->getPossibleEntriesTree(addr);

        DPRINTF(Ztags, "Before calling getVictim() \n");
        // Choose replacement victim from replacement candidates
        CacheBlk* victim = static_cast<CacheBlk*>(replacementPolicy->getVictim(
                                entries));

        // There is only one eviction for this replacement
        evict_blks.push_back(victim);

        return victim;
    }

    /**
     * Insert the new block into the cache and update replacement data.
     *
     * @param pkt Packet holding the address to update
     * @param blk The block to update.
     */
    void insertBlock(const PacketPtr pkt, CacheBlk *blk) override
    {
        // Insert block
        // BaseTags::insertBlock(pkt, blk);

        assert(!blk->isValid());

        // Previous block, if existed, has been removed, and now we have
        // to insert the new one

        // Deal with what we are bringing in
        RequestorID requestor_id = pkt->req->requestorId();
        assert(requestor_id < system->maxRequestors());
        stats.occupancies[requestor_id]++;
        
        // Start inverse tree walk
        /*******************************************************/
        CacheBlk* int_blk=blk;
        DPRINTF(Ztags, "Inside insertBlock() \n");
        // for (const auto& entry : entries) {
        Addr victim_addr = indexingPolicy->regenerateAddr(blk->getTag(), blk);
        Addr parent_addr = pkt->getAddr();
        std::string message = blk->print();
        DPRINTF(Ztags, "Victim -- %s \n", message);
        DPRINTF(Ztags, "Victim -- Address %#x \n", victim_addr);
        DPRINTF(Ztags, "Parent -- Address %#x \n", parent_addr);
        std::vector<ReplaceableEntry*> entries =
            indexingPolicy->getPossibleEntriesBlock(victim_addr);
        std::vector<ReplaceableEntry*> entries2ndlevel =
            indexingPolicy->getPossibleEntriesBlock(parent_addr);


        for (const auto& entry : entries) {
            std::string message = entry->print();
            DPRINTF(Ztags, "Candidates for victim %s \n", message);
        }
        for (const auto& entry : entries2ndlevel) {
            std::string message = entry->print();
            DPRINTF(Ztags, "Candidates for Parent %s \n", message);
        }
        // ReplaceableEntry* victim = candidates[0];
        bool first_level = false;
        for (const auto& entry : entries2ndlevel) {
            DPRINTF(Ztags, "Inside loop first level evaluation\n");
            if((entry->getSet() == blk->getSet()) && (entry->getWay() == blk->getWay())){
                int_blk = blk;
                std::string message = int_blk->print();
                DPRINTF(Ztags, "Intermediate entry on first level: %s \n", message);
                first_level = true;
                break;
            }
        } 

        if(!first_level){
            // uint32_t i = 0;
            Addr temp_addr;
            bool match = false;
            for (const auto& entry : entries2ndlevel) {
                temp_addr = indexingPolicy->regenerateAddr(entry->getTag(), entry);
                std::vector<ReplaceableEntry*> temp_entries = indexingPolicy->getPossibleEntriesBlock(temp_addr);
                for (const auto& entry_temp : temp_entries) {
                    DPRINTF(Ztags, "Inside loop second level evaluation\n");
                    if((entry_temp->getSet() == blk->getSet()) && (entry_temp->getWay() == blk->getWay())){
                        int_blk = static_cast<CacheBlk*>(entry);
                        std::string message = int_blk->print();
                        DPRINTF(Ztags, "Intermediate entry on first level: %s \n", message);
                        match = true;
                        break;
                    }
                } 
                if(match){
                        break;
                    }
            } 
        }

        if(first_level){
            // normal replacement pkt -> blk
            // Insert block with tag, src requestor id and task id
            std::string message = blk->print();
            DPRINTF(Ztags, "1st replacement before -- %s\n", message);
            DPRINTF(Ztags, "Tag to be inserted -- %#x\n", extractTag(pkt->getAddr()));
            blk->insert(extractTag(pkt->getAddr()), pkt->isSecure(), requestor_id,
                        pkt->req->taskId());
            DPRINTF(Ztags, "1st replacement after -- %s\n", message);
            // Check if cache warm up is done
            if (!warmedUp && stats.tagsInUse.value() >= warmupBound) {
                warmedUp = true;
                stats.warmupTick = curTick();
            }

            // We only need to write into one tag and one data block.
            stats.tagAccesses += 1;
            stats.dataAccesses += 1;

            // Increment tag counter
            stats.tagsInUse++;

            // Update replacement policy
            replacementPolicy->reset(blk->replacementData, pkt);
        }
        else{
            // Tree walk replacement pkt -> int_blk -> blk
            // 1st int_blk -> blk
            std::string message = blk->print();
            DPRINTF(Ztags, "1st replacement before -- %s\n", message);
            // if(int_blk->isValid()){
            //     blk = *int_blk;
            // }
            // else{
            blk->insert(int_blk->getTag(), int_blk->isSecure(), requestor_id,
                        pkt->req->taskId());
            if(int_blk->isValid()){
                blk->setCoherenceBits(int_blk->getCoherence());
                blk->setWhenReady(curTick());
                blk->data = int_blk->data;
            }
            // if (int_blk->wasPrefetched()) {
            //     blk->setPrefetched();
            // }
            // blk->setCoherenceBits(int_blk->coherence);
            // moveBlock(int_blk, blk);
            // blk->invalidate();
            message = blk->print();
            DPRINTF(Ztags, "1st replacement after -- %s\n", message);

            // 2nd pkt -> int_blk
            message = int_blk->print();
            DPRINTF(Ztags, "2nd replacement before -- %s\n", message);
            int_blk->invalidate();
            int_blk->insert(extractTag(pkt->getAddr()), pkt->isSecure(), requestor_id,
                        pkt->req->taskId());
            message = int_blk->print();
            DPRINTF(Ztags, "2nd replacement after -- %s\n", message);
            // Check if cache warm up is done
            if (!warmedUp && stats.tagsInUse.value() >= warmupBound) {
                warmedUp = true;
                stats.warmupTick = curTick();
            }

            // We only need to write into one tag and one data block.
            stats.tagAccesses += 2;
            stats.dataAccesses += 2;

            // Increment tag counter
            stats.tagsInUse++;

            // Update replacement policy
            replacementPolicy->reset(int_blk->replacementData, pkt);
            replacementPolicy->reset(blk->replacementData, pkt);
        }
        
    }

    void moveBlock(CacheBlk *src_blk, CacheBlk *dest_blk) override;

    /**
     * Limit the allocation for the cache ways.
     * @param ways The maximum number of ways available for replacement.
     */
    virtual void setWayAllocationMax(int ways) override
    {
        fatal_if(ways < 1, "Allocation limit must be greater than zero");
        allocAssoc = ways;
    }

    /**
     * Get the way allocation mask limit.
     * @return The maximum number of ways available for replacement.
     */
    virtual int getWayAllocationMax() const override
    {
        return allocAssoc;
    }

    /**
     * Regenerate the block address from the tag and indexing location.
     *
     * @param block The block.
     * @return the block address.
     */
    Addr regenerateBlkAddr(const CacheBlk* blk) const override
    {
        return indexingPolicy->regenerateAddr(blk->getTag(), blk);
    }

    void forEachBlk(std::function<void(CacheBlk &)> visitor) override {
        for (CacheBlk& blk : blks) {
            visitor(blk);
        }
    }

    bool anyBlk(std::function<bool(CacheBlk &)> visitor) override {
        for (CacheBlk& blk : blks) {
            if (visitor(blk)) {
                return true;
            }
        }
        return false;
    }
};

} // namespace gem5

#endif //__MEM_CACHE_TAGS_ZBASE_SET_ASSOC_HH__
