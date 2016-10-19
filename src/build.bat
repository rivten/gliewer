@echo off

rem --------------------------------------------------------------------------
rem                        COMPILATION
rem --------------------------------------------------------------------------

ctime -begin gliewer_timings.ctm

set SDLPath=C:\SDL2-2.0.4\
set SDLBinPath=%SDLPath%\lib\x64\

set GLEWPath=C:\glew-1.13.0\
set GLEWBinPath=%GLEWPath%\lib\Release\x64\

set ImGuiPath=..\dep\imgui\

set UntreatedWarnings=/wd4100 /wd4244 /wd4201 /wd4127 /wd4505 /wd4456
set CommonCompilerDebugFlags=/MT /Od /Oi /EHa /WX /W4 %UntreatedWarnings% /Z7 /nologo /I %SDLPath%\include\ /I %GLEWPath%\include\ /I %ImGuiPath% /DRIVTEN_SLOW=1
set CommonLinkerDebugFlags=/incremental:no /opt:ref /subsystem:console %SDLBinPath%\SDL2.lib %SDLBinPath%\SDL2main.lib %GLEWBinPath%\glew32s.lib opengl32.lib /ignore:4099

pushd ..\build\
cl %CommonCompilerDebugFlags% ..\src\sdl_gl_layer.cpp /link %CommonLinkerDebugFlags%
popd

rem --------------------------------------------------------------------------
echo Compilation completed...

ctime -end gliewer_timings.ctm

rem ctime -stats gliewer_timings.ctm
