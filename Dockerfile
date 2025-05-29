# Use Ubuntu 22.04 as base
FROM ubuntu:22.04

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive \
    NODE_VERSION=20 \
    LEAP_VERSION=5.0.3 \
    CDT_VERSION=4.1.0

# Install base dependencies with BuildKit cache
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && apt-get install -y --no-install-recommends \
    wget \
    ca-certificates \
    git \
    build-essential \
    curl \
    cmake \
    make \
    jq \
    && rm -rf /var/lib/apt/lists/*

# Install Node.js LTS
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    curl -fsSL https://deb.nodesource.com/setup_${NODE_VERSION}.x | bash - \
    && apt-get install -y nodejs \
    && npm install -g npm@latest

# Install Antelope Leap
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    mkdir -p /tmp/leap \
    && cd /tmp/leap \
    && wget -q https://github.com/AntelopeIO/leap/releases/download/v${LEAP_VERSION}/leap_${LEAP_VERSION}_amd64.deb \
    && apt-get update \
    && apt-get install -y ./leap_${LEAP_VERSION}_amd64.deb \
    && rm -rf /tmp/leap

# Install Antelope CDT
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    mkdir -p /tmp/cdt \
    && cd /tmp/cdt \
    && wget -q https://github.com/AntelopeIO/cdt/releases/download/v${CDT_VERSION}/cdt_${CDT_VERSION}-1_amd64.deb \
    && apt-get update \
    && apt-get install -y ./cdt_${CDT_VERSION}-1_amd64.deb \
    && rm -rf /tmp/cdt

# Install runtime dependencies
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Set up environment
ENV PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/opt/leap/${LEAP_VERSION}/bin:/usr/opt/cdt/${CDT_VERSION}/bin"
ENV NODE_PATH="/usr/lib/node_modules"
ENV LEAP_PATH="/usr/opt/leap/${LEAP_VERSION}"
ENV CDT_PATH="/usr/opt/cdt/${CDT_VERSION}"

# Create a non-root user with the same UID/GID as the host user
ARG USER_ID=1000
ARG GROUP_ID=1000

# Create a user and group if they don't exist
RUN groupadd -g ${GROUP_ID} developer 2>/dev/null || true \
    && useradd -u ${USER_ID} -g ${GROUP_ID} -m developer 2>/dev/null || true \
    && mkdir -p /workspace/build \
    && mkdir -p /home/developer \
    && chown -R ${USER_ID}:${GROUP_ID} /workspace \
    && chown -R ${USER_ID}:${GROUP_ID} /home/developer \
    && chmod 755 /workspace \
    && chmod 775 /workspace/build

# Set up working directories
WORKDIR /workspace

# Copy package files first to leverage Docker cache
COPY --chown=${USER_ID}:${GROUP_ID} package*.json ./

# Install npm dependencies with cache
RUN --mount=type=cache,target=/home/developer/.npm \
    npm ci --prefer-offline

# Copy the rest of the application
COPY --chown=${USER_ID}:${GROUP_ID} . .

# Make scripts executable
RUN chmod +x *.sh

# Verify installations
RUN echo "Node.js version: $(node --version)" \
    && echo "npm version: $(npm --version)" \
    && echo "Leap version: $(cleos version client)" \
    && echo "CDT version: $(cdt-cpp --version)" \
    && cdt-cpp --version | grep "${CDT_VERSION}" || true

# Install Node.js dependencies for the tests
USER root
RUN cd tests && npm install --unsafe-perm

# Change ownership of node_modules and package files to the non-root user
RUN chown -R ${USER_ID}:${GROUP_ID} /workspace/tests/node_modules \
    /workspace/tests/package*.json \
    /workspace/tests/package-lock.json

# Switch to non-root user for safety
USER ${USER_ID}:${GROUP_ID}

# Set environment variables for the user
ENV HOME=/home/developer
ENV GIT_CONFIG_NOSYSTEM=1

# Create a local git config that won't require root permissions
RUN mkdir -p /home/developer/.config/git \
    && git config --file /home/developer/.gitconfig safe.directory /workspace
