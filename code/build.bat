@echo off

set CommonCompilerFlags= -O2 -MTd -fp:fast -nologo -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -DRPG_INTERNAL=1 -DRPG_SLOW=1 -DRPG_WIN32=1 -FC -Z7 
set CommonLinkerFlags=-incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

REM TODO - can we just build both with one exe?

REM (allen): quit early if we already have cl
where /q cl
IF not %ERRORLEVEL% == 0 (call ..\misc\shell.bat)

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

del *.pdb > NUL 2> NUL

REM Preprocessor
cl %CommonCompilerFlags% -D_CRT_SECURE_NO_WARNINGS ..\rpg\code\preprocessor.cpp /link %CommonLinkerFlags%
pushd ..\rpg\code
..\..\build\preprocessor.exe > generated.h
popd

REM 32-bit build
REM cl %CommonCompilerFlags% w:\rpg\code\win32_rpg.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build
REM Optimization switches /02
echo WAITING FOR PDB > lock.tmp
cl %CommonCompilerFlags% w:\rpg\code\rpg.cpp -Fmrpg.map -LD /link -opt:ref -PDB:rpg_%random%.pdb -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender -EXPORT:GameInitializeMemory
del lock.tmp
cl %CommonCompilerFlags% w:\rpg\code\win32_rpg.cpp -Fmwin32_rpg.map /link %CommonLinkerFlags%
popd

