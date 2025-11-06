#include "NXRGUtil.h"

std::uint16_t NXRGHandle::s_nextIndex = 0;
std::unordered_map<uint16_t, uint16_t> NXRGHandle::s_maxVersions;
