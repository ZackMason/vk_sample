@echo off
if not exist src echo "no source"
if not exist build mkdir build

pushd build

if not exist SDL2.dll echo "No SDL DLL"
if not exist SDL2_mixer.dll echo "No SDL Mixer DLL"

set OptLevel=fast

if exist C:\Users\zack (
set PhysXSDK=C:\Users\zack\Documents\GitHub\Physx5\PhysX\physx
set PhysXInclude=/I %PhysXSDK%/include
)
if exist C:\Users\crazy (
set PhysXSDK=C:\Users\crazy\Documents\GitHub\Physx5\PhysX\physx
set PhysXInclude=/I %PhysXSDK%\include
)
Set PhysXCompiler=%PhysXSDK%\bin\win.x86_64.vc142.md
rem use physx checked or debug for development
set PhysXOpt=release
set PhysXLinkLibs=PhysX_64.lib PhysXCommon_64.lib PhysXCooking_64.lib PhysXFoundation_64.lib PhysXExtensions_static_64.lib PhysXCharacterKinematic_static_64.lib PhysXPvdSDK_static_64.lib PhysXVehicle_static_64.lib

if "%OptLevel%"=="slow" (
    set OptimizationFlags=/DEBUG:full /Zi /O0 /fp:fast /arch:AVX2
) else (
    set OptimizationFlags=/DNDEBUG /O2 /fp:fast /arch:AVX2
)

set IncludeFlags=/I ..\include /I ..\include\vendor /I..\include\vendor\SDL /I %VULKAN_SDK%/include 
set CompilerFlags=-nologo -FC -WX -W4 -wd4100 -wd4201 -wd4702 -wd4701 -wd4189 -MD -EHsc /std:c++20
set SDLLinkFlags=SDL2.lib SDL2_mixer.lib SDL2main.lib
set LinkFlags=-incremental:no -opt:ref user32.lib gdi32.lib shell32.lib /LIBPATH:%VULKAN_SDK%/lib /LIBPATH:..\lib\debug vulkan-1.lib
set PhysicsLinkFlags=-opt:ref user32.lib gdi32.lib shell32.lib /LIBPATH:%PhysXCompiler%/%PhysXOpt% %PhysXLinkLibs%

xcopy %PhysXCompiler%\%PhysXOpt%\*.dll . /yq

if "%~2"=="game" (
    del *.pdb > NUL 2> NUL
    echo Build Lock > lock.tmp
    cl %OptimizationFlags% -DGEN_INTERNAL=1 %IncludeFlags% /I ..\src %CompilerFlags% ..\src\app_build.cpp -LD /link -PDB:game_%random%.pdb %LinkFlags%
    del lock.tmp
)

if "%~1"=="win32" (
    cl %OptimizationFlags% -DGEN_INTERNAL=0 %IncludeFlags% %CompilerFlags% ..\src\app_platform.cpp /link %LinkFlags% %SDLLinkFlags% glfw3.lib /OUT:game.exe
)

if "%~1"=="physics" (
    cl %OptimizationFlags% -DGEN_LINK_PHYSICS_API_PHYSX=1 -DGEN_INTERNAL=1 %IncludeFlags% %PhysXInclude% %CompilerFlags% ..\src\app_physics.cpp -LD /link %PhysicsLinkFlags%
    rem cl %OptimizationFlags% -DGEN_LINK_PHYSICS_API_PHYSX=0 -DGEN_INTERNAL=0 %IncludeFlags% /I %PhysXInclude% %CompilerFlags% ..\src\app_physics.cpp -LD 
)

if "%~1"=="tests" (
    cl %OptimizationFlags% -DGEN_INTERNAL=0 %IncludeFlags% %CompilerFlags% ..\tests\tests.cpp /link %LinkFlags% %SDLLinkFlags%
)

popd build