#ifndef COMBIMGSAMPLERSPLITTER_H
#define COMBIMGSAMPLERSPLITTER_H

#include "core/templates/vector.h"

// HACK: In the SPIR-V bytecode, split each combined image sampler into a texture and sampler.
Vector<uint32_t> combimgsampsplitter(const Vector<uint32_t> &in_spv);

#endif // COMBIMGSAMPLERSPLITTER_H
