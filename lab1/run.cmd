@echo off
echo === Сборка проекта ===

REM обновление кода
git pull

REM Переход в директорию проекта
set PROJECT_DIR = %~dp0
cd "%PROJECT_DIR%"

if exist "build" (
    rmdir /s /q "build"
)

mkdir build
cd build

cmake -G "MinGW Makefiles" ..
mingw32-make