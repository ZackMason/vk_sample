#pragma once


export_fn(void) enumerate_collision_callbacks(const char** buffer, u32* count) {
    #define export_name(name) if (buffer) { buffer[i++] = #name; } else { i++; }

    u32 i = 0;

    export_name(on_trigger_set_light_probe);
    export_name(on_trigger_pickup_item);
    
    *count = i;

    #undef export_name
}
