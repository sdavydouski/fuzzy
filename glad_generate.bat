@echo off

python externals\glad\main.py --profile="core" --api="gl=3.3" --generator="c-debug" --spec="gl" --extensions="" --omit-khrplatform --reproducible --out-path=".\generated\glad"
