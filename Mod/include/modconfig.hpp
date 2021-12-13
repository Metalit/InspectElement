#pragma once

#define HAS_CODEGEN
#include "extern/config-utils/shared/config-utils.hpp"

DECLARE_CONFIG(ModConfig,

    // 0: side trigger, 1: lower button, 2: top button, 3: none
    CONFIG_VALUE(Button, int, "Inspect Button", 0);

    CONFIG_INIT_FUNCTION(
        CONFIG_INIT_VALUE(Button);
    )
)