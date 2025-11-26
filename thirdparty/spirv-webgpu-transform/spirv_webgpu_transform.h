#ifndef COMBIMGSAMPSPLITTER_H
#define COMBIMGSAMPSPLITTER_H

#include <stdint.h>

extern "C" {
    void combimgsampsplitter_alloc(uint32_t* in_spv, uint32_t in_count, uint32_t** out_spv, uint32_t* out_count);
    void combimgsampsplitter_free(uint32_t* out_spv);
    void dreftexturesplitter_alloc(uint32_t* in_spv, uint32_t in_count, uint32_t** out_spv, uint32_t* out_count);
    void dreftexturesplitter_free(uint32_t* out_spv);
}

#endif
