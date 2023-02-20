#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "App/vk_state.cpp"
#include "App/app.cpp"


// note(zack):
// this isn't really helpful but I will leave it here.

// #if _WIN32

// #define WIN32_LEAN_AND_MEAN
// #define NOMINMAX
// #include "Windows.h"
// #undef near
// #undef far

// BOOL APIENTRY DllMain(
//     HMODULE hModule,
//     DWORD  ul_reason_for_call,
//     LPVOID lpReserved
// ) {
//     switch (ul_reason_for_call) {
//         case DLL_PROCESS_ATTACH:
//             gen_info("dll_main", "DLL Load Process");
//             break;
//         case DLL_THREAD_ATTACH:
//             gen_info("dll_main", "DLL Load Threaded Process");
//             break;
//         case DLL_THREAD_DETACH:
//             gen_info("dll_main", "DLL Unload Threaded Process");
//             break;
//         case DLL_PROCESS_DETACH:
//             gen_info("dll_main", "DLL Unload Process");
//             break;
//     }
//     return TRUE;
// }

// #endif
