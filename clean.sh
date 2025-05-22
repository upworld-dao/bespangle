#!/bin/bash
set -e

echo "Cleaning build directories..."

# Remove any existing build directories
rm -rf ./build
rm -rf ./org/build

echo "Clean complete. All build directories have been removed."
