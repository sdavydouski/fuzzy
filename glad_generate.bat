@echo off

python externals\glad\main.py --profile="core" --api="gl=3.3" --generator="c-debug" --spec="gl" --extensions="" --omit-khrplatform --reproducible --out-path=".\generated\glad"

set build_dir=".\build\libs"

if not exist %build_dir% mkdir %build_dir%
pushd %build_dir%
cl -DGLAD_GLAPI_EXPORT -DGLAD_GLAPI_EXPORT_BUILD -MDd -Gd  -Zi -Od -I"..\..\generated\glad\include" "..\..\generated\glad\src\glad.c" -LDd /link
popd
