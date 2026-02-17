#!/bin/bash

echo "=== Сборка проекта ==="

# Переход в директорию проекта
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_DIR"

if [ -d "build" ]; then
    rm -rf build
fi

mkdir build
cd build

cmake ..
make