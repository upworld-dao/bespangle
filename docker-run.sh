#!/bin/bash
set -e

# Ensure we're in the project root
cd "$(dirname "$0")"

# Default command to run if none specified
DEFAULT_COMMAND="./run_tests.sh"

# Parse command line arguments
COMMAND="$DEFAULT_COMMAND"
CLEAN_BUILD=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --command=*)
            COMMAND="${1#*=}"
            shift
            ;;
        --help)
            echo "Usage: $0 [options] [-- command [args...]]"
            echo "Options:"
            echo "  --clean          Clean build directory before running"
            echo "  --command=CMD    Command to run in the container (default: $DEFAULT_COMMAND)"
            echo "  --help           Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0 --clean"
            echo "  $0 --command='./build.sh'"
            echo "  $0 --command='./deploy.sh -a deploy'"
            exit 0
            ;;
        --)
            shift
            COMMAND="$*"
            break
            ;;
        *)
            # If no --command= is specified but there are arguments, use them as the command
            if [ "$COMMAND" = "$DEFAULT_COMMAND" ] && [ "$1" != "" ]; then
                COMMAND="$*"
                break
            else
                echo "Unknown option: $1"
                exit 1
            fi
            ;;
    esac
done

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
chown -R $USER_ID:$GROUP_ID "$(pwd)/build"
chmod -R 755 "$(pwd)/build"

# Clean up build artifacts if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo "Cleaning build artifacts..."
    rm -rf "$(pwd)/build/"* 2>/dev/null || true
fi

# Create a temporary directory for the container's home
export CONTAINER_HOME="/tmp/container_home_$USER_ID"
mkdir -p "$CONTAINER_HOME"
chmod 755 "$CONTAINER_HOME"

# Create a local .gitconfig file for the container
cat > "$CONTAINER_HOME/.gitconfig" << 'EOL'
[safe]
    directory = /workspace
EOL

echo "Running in Docker container: $COMMAND"

# Run the container with the current user's UID/GID
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
    bash -c "mkdir -p /workspace/build && chmod 755 /workspace/build && $COMMAND"

# Clean up the temporary directory
rm -rf "$CONTAINER_HOME"
