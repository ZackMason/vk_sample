@echo off
if not exist src echo "no source"
if not exist build mkdir build

pushd build

set OptimizationFlags=/DEBUG:FULL /Zi /fp:fast /arch:AVX
set IncludeFlags=/I ..\include /I ..\include\vendor /I..\include\vendor\SDL /I %VULKAN_SDK%/include
set CompilerFlags=-nologo -FC -WX -W4 -wd4100 -wd4201 -wd4702 -wd4701 -wd4189 -MDd -EHsc /std:c++20
set SDLLinkFlags=SDL2.lib SDL2_mixer.lib SDL2main.lib
set LinkFlags=-opt:ref user32.lib gdi32.lib winmm.lib shell32.lib /LIBPATH:%VULKAN_SDK%/lib /LIBPATH:..\lib\debug vulkan-1.lib

if "%~2"=="game" (
    del *.pdb > NUL 2> NUL
    echo Build Lock > lock.tmp
    cl %OptimizationFlags% %IncludeFlags% /I ..\src %CompilerFlags% ..\src\app_build.cpp -LD /link %LinkFlags%
    del lock.tmp
)

if "%~1"=="win32" (
    cl %OptimizationFlags% %IncludeFlags% %CompilerFlags% ..\src\app_platform.cpp /link %LinkFlags% %SDLLinkFlags% glfw3.lib
)

popd build