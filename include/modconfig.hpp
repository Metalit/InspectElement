#pragma once
#include "extern/config-utils/shared/config-utils.hpp"

DECLARE_CONFIG(ModConfig,

    CONFIG_VALUE(Active, bool, "Mod Active", true);

    CONFIG_INIT_FUNCTION(
        CONFIG_INIT_VALUE(Active);
    )
)