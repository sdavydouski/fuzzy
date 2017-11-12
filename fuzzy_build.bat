@echo off

set build_dir="build"
set libs_dir=%build_dir%\libs

if not exist %build_dir% mkdir %build_dir%
pushd %build_dir%
cl -MT -I ..\externals\glfw\include -Zi ..\src\fuzzy.cpp ..\%libs_dir%\glfw3.lib opengl32.lib shell32.lib gdi32.lib user32.lib
popd
