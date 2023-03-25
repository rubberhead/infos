/* B171926 */
#include <infos/drivers/ata/page-cache.h>

using namespace infos::util; 
using namespace infos::drivers::ata; 

struct Cache::_Block {

    /* Fields */
    uint8_t* data_base; 
    size_t   offset_arg; 

    /* Methods */
    _Block() = default; 
    _Block(
        const size_t offset, 
        const size_t block_size
    ):
        offset_arg(offset), 
        data_base(new uint8_t[block_size]) {}
    ~_Block() { 
        // Can't really delete... Make complains about delete[] void* impl. 
        // delete[] data_base; 
    }

    constexpr bool is_allocated() const { return data_base != NULL; }
}; 

Cache::Cache(const size_t block_size): 
_block_size(block_size), 
_blocks() {}

uint8_t* Cache::push_new_block(const size_t offset) {
    if (this->_blocks.count() == _CACHE_SIZE) {
        // => Cache full, apply FIFO
        _Block* victim_blk = _blocks.pop(); 
        victim_blk->offset_arg = offset; 
        _blocks.append(victim_blk); // push at front, append at rear, etc etc.
        return victim_blk->data_base; 
    }
    // => Cache not full, allocate and append
    _Block* new_blk = new _Block(offset, this->_block_size); 
    uint8_t* res = new_blk->data_base; 
    this->_blocks.append(new_blk); 
    return res; 
}

uint8_t* Cache::find_block_in_cache(const size_t offset) {
    for (const auto block : this->_blocks) {
        if (block->offset_arg == offset) {
            return block->data_base; 
        }
    }
    return NULL; 
}