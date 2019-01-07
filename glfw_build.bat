@echo off

set build_dir=".\build\glfw"
set libs_dir=".\build\libs"

if not exist %build_dir% mkdir %build_dir%
pushd %build_dir%
cmake -D GLFW_BUILD_DOCS=OFF -D GLFW_BUILD_TESTS=OFF -D GLFW_BUILD_EXAMPLES=OFF -D USE_MSVC_RUNTIME_LIBRARY_DLL=ON ..\..\externals\glfw -G "Visual Studio 15 2017 Win64" 
devenv .\GLFW.sln -rebuild Debug 
popd

if not exist %libs_dir% mkdir %libs_dir%
pushd %libs_dir%
copy ..\glfw\src\Debug\glfw3.lib .\
copy ..\glfw\src\glfw.dir\Debug\glfw.pdb .\
popd
