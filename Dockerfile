# Use Ubuntu 22.04 as base
FROM ubuntu:22.04

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive \
    NODE_VERSION=20 \
    LEAP_VERSION=5.0.3 \
    CDT_VERSION=4.1.0 \
    NPM_CONFIG_UPDATE_NOTIFIER=false \
    NPM_CONFIG_CACHE=/tmp/npm-cache \
    NODE_PATH=/usr/lib/node_modules

# Set path-related environment variables after LEAP and CDT are installed
ENV LEAP_PATH=/usr/opt/leap/5.0.3 \
    CDT_PATH=/usr/opt/cdt/4.1.0 \
    PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/opt/leap/5.0.3/bin:/usr/opt/cdt/4.1.0/bin"

# Create non-root user early for better layer caching
ARG USER_ID=1000
ARG GROUP_ID=1000
RUN groupadd -g ${GROUP_ID} developer 2>/dev/null || true \
    && useradd -u ${USER_ID} -g ${GROUP_ID} -m developer 2>/dev/null || true

# Install base dependencies with BuildKit cache and cleanup in one layer
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    curl \
    wget \
    git \
    build-essential \
    cmake \
    make \
    jq \
    gnupg \
    && rm -rf /var/lib/apt/lists/*

# Install Node.js from NodeSource with verification
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    mkdir -p /etc/apt/keyrings \
    && curl -fsSL https://deb.nodesource.com/gpgkey/nodesource-repo.gpg.key | gpg --dearmor -o /etc/apt/keyrings/nodesource.gpg \
    && echo "deb [signed-by=/etc/apt/keyrings/nodesource.gpg] https://deb.nodesource.com/node_${NODE_VERSION}.x nodistro main" > /etc/apt/sources.list.d/nodesource.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends nodejs \
    && npm install -g npm@latest \
    && npm cache clean --force \
    && rm -rf /var/lib/apt/lists/*

# Install Antelope Leap and CDT in one layer
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    mkdir -p /tmp/install \
    && cd /tmp/install \
    # Install Leap
    && wget -q https://github.com/AntelopeIO/leap/releases/download/v${LEAP_VERSION}/leap_${LEAP_VERSION}_amd64.deb \
    && apt-get update \
    && apt-get install -y ./leap_${LEAP_VERSION}_amd64.deb \
    # Install CDT
    && wget -q https://github.com/AntelopeIO/cdt/releases/download/v${CDT_VERSION}/cdt_${CDT_VERSION}-1_amd64.deb \
    && apt-get install -y ./cdt_${CDT_VERSION}-1_amd64.deb \
    # Cleanup
    && apt-get clean \
    && rm -rf /tmp/install /var/lib/apt/lists/*

# Set up workspace and permissions
RUN mkdir -p /workspace/build /home/developer \
    && chown -R ${USER_ID}:${GROUP_ID} /workspace /home/developer \
    && chmod 755 /workspace \
    && chmod 775 /workspace/build

# Set working directory
WORKDIR /workspace

# Create tests directory and copy test dependencies
RUN mkdir -p ./tests
COPY --chown=${USER_ID}:${GROUP_ID} tests/package*.json ./tests/

# Install test dependencies as root
RUN cd ./tests && \
    npm install --prefer-offline --no-audit --no-fund \
    && npm cache clean --force

# Switch to non-root user after installation
USER ${USER_ID}:${GROUP_ID}

# Copy application code
COPY --chown=${USER_ID}:${GROUP_ID} . .

# Make scripts executable and verify installations
RUN chmod +x *.sh \
    && echo "Node.js version: $(node --version)" \
    && echo "npm version: $(npm --version)" \
    && echo "Leap version: $(cleos version client)" \
    && echo "CDT version: $(cdt-cpp --version)" \
    && cdt-cpp --version | grep "${CDT_VERSION}" || true

# Install test dependencies
USER root
RUN --mount=type=cache,target=/home/developer/.npm \
    cd tests \
    && npm ci --prefer-offline --no-audit --no-fund \
    && chown -R ${USER_ID}:${GROUP_ID} node_modules package*.json

# Switch back to non-root user and set final environment
USER ${USER_ID}:${GROUP_ID}
ENV HOME=/home/developer \
    GIT_CONFIG_NOSYSTEM=1

# Configure git to work without system config
RUN mkdir -p /home/developer/.config/git \
    && git config --file /home/developer/.gitconfig safe.directory /workspace

# Health check to verify services are running
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD node --version && cleos version client && cdt-cpp --version
