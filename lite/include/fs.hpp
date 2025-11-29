#pragma once

#include <daisy_seed.h>
#include <ff.h>

#include "metadata.hpp"

namespace fs {
void init();
void Tree(const char *path, size_t depth);
deloop::Metadata &GetMetadata();
}  // namespace fs
