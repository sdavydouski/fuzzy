@echo off

glad --profile="core" --api="gl=3.3" --generator="c-debug" --spec="gl" --extensions="" --omit-khrplatform --out-path=".\generated\glad"
