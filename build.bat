@echo off

set build_dir=".\build"

if not exist %build_dir% mkdir %build_dir%
pushd %build_dir%
cl -Zi ..\src\test.cpp
popd
