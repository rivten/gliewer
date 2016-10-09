@echo off

set STARTTIME=%TIME%

rem --------------------------------------------------------------------------
rem                        COMPILATION
rem --------------------------------------------------------------------------

set SDLPath=C:\SDL2-2.0.4\
set SDLBinPath=%SDLPath%\lib\x64\

set GLEWPath=C:\glew-1.13.0\
set GLEWBinPath=%GLEWPath%\lib\Release\x64\

set UntreatedWarnings=/wd4100 /wd4244 /wd4201 /wd4127 /wd4505 /wd4456
set CommonCompilerDebugFlags=/MT /Od /Oi /EHa /WX /W4 %UntreatedWarnings% /Z7 /nologo /I %SDLPath%\include\ /I %GLEWPath%\include\ /DRIVTEN_SLOW=1
set CommonLinkerDebugFlags=/incremental:no /opt:ref /subsystem:console %SDLBinPath%\SDL2.lib %SDLBinPath%\SDL2main.lib %GLEWBinPath%\glew32s.lib opengl32.lib /ignore:4099

pushd ..\build\
cl %CommonCompilerDebugFlags% ..\src\sdl_gl_layer.cpp /link %CommonLinkerDebugFlags%
popd

rem --------------------------------------------------------------------------
echo Compilation completed...

rem Code used to be found here : http://stackoverflow.com/questions/9922498/calculate-time-difference-in-windows-batch-file

set ENDTIME=%TIME%

rem convert STARTTIME and ENDTIME to centiseconds
set /A STARTTIME=(1%STARTTIME:~0,2%-100)*360000 + (1%STARTTIME:~3,2%-100)*6000 + (1%STARTTIME:~6,2%-100)*100 + (1%STARTTIME:~9,2%-100)
set /A ENDTIME=(1%ENDTIME:~0,2%-100)*360000 + (1%ENDTIME:~3,2%-100)*6000 + (1%ENDTIME:~6,2%-100)*100 + (1%ENDTIME:~9,2%-100)

rem calculating the duration is easy
set /A DURATION=%ENDTIME%-%STARTTIME%

rem we might have measured the time inbetween days
if(%ENDTIME% LSS %STARTTIME%) set set /A DURATION=%STARTTIME%-%ENDTIME%

rem now break the centiseconds down to hors, minutes, seconds and the remaining centiseconds
set /A DURATIONH=%DURATION% / 360000
set /A DURATIONM=(%DURATION% - %DURATIONH%*360000) / 6000
set /A DURATIONS=(%DURATION% - %DURATIONH%*360000 - %DURATIONM%*6000) / 100
set /A DURATIONHS=(%DURATION% - %DURATIONH%*360000 - %DURATIONM%*6000 - %DURATIONS%*100)

rem some formatting
if %DURATIONH% LSS 10 set DURATIONH=0%DURATIONH%
if %DURATIONM% LSS 10 set DURATIONM=0%DURATIONM%
if %DURATIONS% LSS 10 set DURATIONS=0%DURATIONS%
if %DURATIONHS% LSS 10 set DURATIONHS=0%DURATIONHS%

rem outputing
echo %DURATIONH%h:%DURATIONM%m:%DURATIONS%s,%DURATIONHS%

endlocal
goto :EOF
