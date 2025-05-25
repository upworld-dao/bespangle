# Use Ubuntu 22.04 as base
FROM ubuntu:22.04

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Install base dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    wget \
    ca-certificates \
    git \
    build-essential \
    curl \
    cmake \
    make \
    jq \
    && rm -rf /var/lib/apt/lists/*

# Install Node.js 20 LTS (latest stable)
RUN curl -fsSL https://deb.nodesource.com/setup_20.x | bash - \
    && apt-get install -y nodejs \
    && rm -rf /var/lib/apt/lists/*

# Install latest npm
RUN npm install -g npm@latest

# Install Antelope Leap
RUN mkdir -p /tmp/leap \
    && cd /tmp/leap \
    && wget -q https://github.com/AntelopeIO/leap/releases/download/v5.0.3/leap_5.0.3_amd64.deb \
    && apt-get update \
    && apt-get install -y ./leap_5.0.3_amd64.deb \
    && rm -rf /tmp/leap \
    && rm -rf /var/lib/apt/lists/*

# Install Antelope CDT
ARG CDT_VERSION=4.1.0
RUN mkdir -p /tmp/cdt \
    && cd /tmp/cdt \
    && wget -q https://github.com/AntelopeIO/cdt/releases/download/v${CDT_VERSION}/cdt_${CDT_VERSION}-1_amd64.deb \
    && apt-get update \
    && apt-get install -y ./cdt_${CDT_VERSION}-1_amd64.deb \
    && rm -rf /tmp/cdt \
    && rm -rf /var/lib/apt/lists/*

# Set up working directory
WORKDIR /app

# Copy project files
COPY . .

# Make scripts executable
RUN chmod +x *.sh

# Set environment variables
ENV PATH="/usr/opt/leap/5.0.3/bin:/usr/opt/cdt/${CDT_VERSION}/bin:${PATH}"

# Verify installations
RUN echo "Leap version: $(cleos version client)" \
    && echo "CDT version: $(cdt-cpp --version)"

# Install base utilities
RUN apt-get update && apt-get install -y --no-install-recommends \
    # Add other base utilities here if needed
    && rm -rf /var/lib/apt/lists/*

# eosio.bios download/build steps removed

# Add CDT binaries to the system PATH
ENV PATH="/usr/opt/cdt/${CDT_VERSION}/bin:${PATH}"

# Verify installations
RUN echo "Node.js version: $(node --version)"
RUN echo "npm version: $(npm --version)"
RUN echo "CDT version: $(cdt-cpp --version)" && cdt-cpp --version | grep "${CDT_VERSION}"

# Create a non-root user with the same UID/GID as the host user
ARG USER_ID=1000
ARG GROUP_ID=1000

# Create a user and group if they don't exist
RUN groupadd -g ${GROUP_ID} developer 2>/dev/null || true \
    && useradd -u ${USER_ID} -g ${GROUP_ID} -m developer 2>/dev/null || true \
    && mkdir -p /workspace \
    && mkdir -p /home/developer \
    && chown -R ${USER_ID}:${GROUP_ID} /workspace \
    && chown -R ${USER_ID}:${GROUP_ID} /home/developer

# Set up a working directory inside the container
WORKDIR /workspace

# Switch to the non-root user
USER ${USER_ID}:${GROUP_ID}

# Set environment variables for the user
ENV HOME=/home/developer
ENV GIT_CONFIG_NOSYSTEM=1

# Create a local git config that won't require root permissions
RUN mkdir -p /home/developer/.config/git \
    && git config --file /home/developer/.gitconfig safe.directory /workspace

# Copy the entire project context into the image
COPY . /workspace/

# Install Node.js dependencies for the tests
# First install as root to avoid permission issues
USER root
RUN cd tests && npm install --unsafe-perm

# Change ownership of node_modules and package files to the non-root user
RUN chown -R ${USER_ID}:${GROUP_ID} /workspace/tests/node_modules \
    && chown -R ${USER_ID}:${GROUP_ID} /workspace/tests/package*.json \
    && chown -R ${USER_ID}:${GROUP_ID} /workspace/tests/package-lock.json

# Switch back to non-root user for safety
USER ${USER_ID}:${GROUP_ID}

# Indicate that the container is ready (optional, can be removed)
# CMD ["echo", "Antelope CDT & Node.js environment ready."]
