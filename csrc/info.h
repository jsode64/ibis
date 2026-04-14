#pragma once

#include "def.h"
#include <vulkan/vulkan.h>

/// The Ibis library version.
static const Version IBIS_VERSION = {
    .major = 1,
    .minor = 0,
    .patch = 0,
};

/// The crate version as a Vulkan version.
static const u32 IBIS_VK_VERSION =
    VK_MAKE_API_VERSION(0, IBIS_VERSION.major, IBIS_VERSION.minor, IBIS_VERSION.patch);
