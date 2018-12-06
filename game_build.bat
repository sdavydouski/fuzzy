@echo off

set include_paths=-I..\..\..\generated\glad\include\ -I..\..\..\externals\stb\ -I..\..\..\externals\glm\ -I..\..\..\externals\json\include\ -I..\..\..\externals\glfw\include\
set compiler_flags=%include_paths% -MDd -nologo -Od -W3 -Zi -EHsc
set linker_flags= -incremental:no -pdb:%random%_fuzzy.pdb opengl32.lib
set build_dir=".\build\Debug\x64"
set lock_file_name="lock.tmp"

if not exist %build_dir% mkdir %build_dir%

pushd %build_dir%
del *fuzzy.pdb > nul 2> nul
echo "Waiting for pdb..." > %lock_file_name%
cl %compiler_flags% "..\..\..\src\fuzzy\fuzzy.cpp" -LDd /link %linker_flags%
del %lock_file_name%
popd