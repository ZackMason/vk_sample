#include <Detour/DetourAlloc.cpp>
#include <Detour/DetourAssert.cpp>
#include <Detour/DetourCommon.cpp>
#include <Detour/DetourNavMesh.cpp>
#include <Detour/DetourNavMeshBuilder.cpp>
#include <Detour/DetourNavMeshQuery.cpp>
#include <Detour/DetourNode.cpp>

#include <Recast/Recast.cpp>
#include <Recast/RecastAlloc.cpp>
#include <Recast/RecastArea.cpp>
#include <Recast/RecastAssert.cpp>
#include <Recast/RecastContour.cpp>
#include <Recast/RecastFilter.cpp>
#include <Recast/RecastLayers.cpp>
#include <Recast/RecastMesh.cpp>
#include <Recast/RecastMeshDetail.cpp>
#include <Recast/RecastRasterization.cpp>
#include <Recast/RecastRegion.cpp>


#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui.cpp"
#include "imgui_demo.cpp"
#include "imgui_draw.cpp"
#include "imgui_tables.cpp"
#include "imgui_widgets.cpp"

#include "imgui_impl_vulkan.cpp"
// #include "imgui_impl_glfw.cpp"

