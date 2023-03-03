@echo off
if not exist src echo "no source"
if not exist build mkdir build

pushd build

if not exist SDL2.dll echo "No SDL DLL"
if not exist SDL2_mixer.dll echo "No SDL Mixer DLL"

set PhysXSDK=C:\Users\crazy\Downloads\PhysX-brickadia-4.1.2\PhysX-brickadia-4.1.2\physx
set PhysXInclude=%PhysXSDK%/include
Set PhysXCompiler=%PhysXSDK%\bin\win.x86_64.vc143.md
rem use checked or debug for development
set PhysXOpt=debug
set PhysXLinkLibs=PhysX_64.lib PhysXCommon_64.lib PhysXCooking_64.lib PhysXFoundation_64.lib PhysXExtensions_static_64.lib PhysXCharacterKinematic_static_64.lib PhysXPvdSDK_static_64.lib PhysXVehicle_static_64.lib

rem set OptimizationFlags=/fp:fast /arch:AVX2 /O2
set OptimizationFlags=/DEBUG:FULL /Zi /fp:fast /arch:AVX2
set IncludeFlags=/I ..\include /I ..\include\vendor /I..\include\vendor\SDL /I %VULKAN_SDK%/include /I %PhysXInclude%
set CompilerFlags=-nologo -FC -WX -W4 -wd4100 -wd4201 -wd4702 -wd4701 -wd4189 -MD -EHsc /std:c++20
set SDLLinkFlags=SDL2.lib SDL2_mixer.lib SDL2main.lib
set LinkFlags=-opt:ref user32.lib gdi32.lib winmm.lib shell32.lib /LIBPATH:%VULKAN_SDK%/lib /LIBPATH:..\lib\debug vulkan-1.lib /LIBPATH:%PhysXCompiler%/%PhysXOpt% %PhysXLinkLibs%

if "%~2"=="game" (
    del *.pdb > NUL 2> NUL
    echo Build Lock > lock.tmp
    cl %OptimizationFlags% %IncludeFlags% /I ..\src %CompilerFlags% ..\src\app_build.cpp -LD /link %LinkFlags%
    del lock.tmp
)

if "%~1"=="win32" (
    cl %OptimizationFlags% %IncludeFlags% %CompilerFlags% ..\src\app_platform.cpp /link %LinkFlags% %SDLLinkFlags% glfw3.lib /OUT:game.exe
)

if "%~1"=="tests" (
    cl %OptimizationFlags% %IncludeFlags% %CompilerFlags% ..\tests\tests.cpp /link %LinkFlags% %SDLLinkFlags%
)

popd build