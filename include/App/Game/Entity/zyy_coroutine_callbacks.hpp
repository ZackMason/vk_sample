#pragma once

export_fn(void) enumerate_coroutine_callbacks(const char** buffer, u32* count) {
    #define export_name(name) if (buffer) { buffer[i++] = #name; } else { i++; }

    u32 i = 0;

    export_name(co_platform);
    
    *count = i;

    #undef export_name
}
