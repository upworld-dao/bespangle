#!/bin/bash
set -e

# Ensure we're in the project root
cd "$(dirname "$0")"

# Build the Docker image if not already built
if ! docker image inspect bespangle-dev-env >/dev/null 2>&1; then
    echo "Building Docker image..."
    docker build -t bespangle-dev-env .
fi

# Get the current user and group IDs to ensure consistent file permissions
USER_ID=$(id -u)
GROUP_ID=$(id -g)

# Ensure the build directory exists and has the right permissions
mkdir -p "$(pwd)/build"
chmod -R a+rw "$(pwd)/build"

# Clean up any existing build artifacts
echo "Cleaning previous build artifacts..."
rm -rf "$(pwd)/build/"* 2>/dev/null || true

# Run the container with the project directory mounted
echo "Running in Docker container..."
# Create a temporary directory for the container's home
export CONTAINER_HOME="/tmp/container_home_$USER_ID"
mkdir -p "$CONTAINER_HOME"
chmod 777 "$CONTAINER_HOME"

# Create a local .gitconfig file for the container
cat > "$CONTAINER_HOME/.gitconfig" << 'EOL'
[safe]
    directory = /workspace
EOL

echo "Running in Docker container..."
docker run --rm -it \
    -v "$(pwd):/workspace" \
    -v "$CONTAINER_HOME:/home/developer" \
    -w /workspace \
    -e BESPANGLE_IN_DOCKER=true \
    -e USER_ID=$USER_ID \
    -e GROUP_ID=$GROUP_ID \
    -e HOME=/home/developer \
    -e GIT_CONFIG_NOSYSTEM=1 \
    --user "$USER_ID:$GROUP_ID" \
    bespangle-dev-env \
    bash -c "chmod -R a+rw /workspace/build/ && ./run_tests.sh $@"

# Clean up the temporary directory
rm -rf "$CONTAINER_HOME"
