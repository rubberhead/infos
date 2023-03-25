/* B171926 */
#pragma once

#include <infos/util/list.h>
#include <infos/drivers/ata/ata-controller.h> // for ata_log

#define CACHE_SIZE 64;  // 64 entries
static constexpr size_t _CACHE_SIZE = CACHE_SIZE; 

namespace infos::drivers::ata {
    /**
     * The FIFO ATA-device block cache. 
     */
    struct Cache {
    public: 
        Cache()  = delete; 
        /**
         * Construct a new in-memory block cache for some ATA block device. 
         * 
         * @param block_size Size of block for this device. 
         */
        Cache(const size_t block_size); 
        ~Cache() = default; 

        /**
         * Try to allocate a new block into cache, or reuse the oldest block in FIFO order. 
         * 
         * Parameters correspond to those of ATADevice::readblocks. 
         * 
         * @return uint8_t* pointer to allocated cache slot (one block); 
         * @return NULL if allocation failed (e.g., out of memory)
         */
        uint8_t* push_new_block(const size_t offset); 
        
        /**
         * Try to find an existing block in cache corresponding to given arguments. 
         * 
         * Parameters correspond to those of ATADevice::readblocks. 
         * 
         * @return uint8_t* pointer to corresponding cache if found; 
         * @return NULL otherwise
         */
        uint8_t* find_block_in_cache(const size_t offset); 

    private: 
        /**
         * Data structure to identify a block entry in cache. 
         */
        struct _Block; 

        infos::util::List<_Block*> _blocks; 
        const size_t _block_size; 
    }; 
}

/* [RANT]
 * Using `List<_Block>` gave me wicked multi-definition errors. 
 * This is fixed by separating impl and header... somehow. 
 * 
 * C++ is hard, what else can I say
 */