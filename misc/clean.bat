@echo off

set build_dir=".\build"
set generated_dir=".\generated"

if exist %build_dir% rmdir /s /q %build_dir%
if exist %generated_dir% rmdir /s /q %generated_dir%
