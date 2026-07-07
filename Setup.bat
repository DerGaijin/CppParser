@echo off
cd /d "%~dp0"
"Build Scripts\premake5.exe" --file="Build Scripts\premake5.lua" vs2022
pause