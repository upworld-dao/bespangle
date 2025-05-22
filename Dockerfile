# Use a minimal Ubuntu base image
FROM ubuntu:22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install base dependencies and Node.js prerequisites
RUN apt-get update && apt-get install -y --no-install-recommends \
    wget \
    ca-certificates \
    git \
    build-essential \
    curl \
    cmake \
    make \
    && rm -rf /var/lib/apt/lists/*

# Install Node.js v18 LTS
RUN curl -fsSL https://deb.nodesource.com/setup_22.x | bash - \
    && apt-get install -y nodejs \
    && rm -rf /var/lib/apt/lists/*

# Install Antelope CDT v4.1.0
ARG CDT_VERSION=4.1.0
ARG CDT_PKG_URL=https://github.com/AntelopeIO/cdt/releases/download/v${CDT_VERSION}/cdt_${CDT_VERSION}-1_amd64.deb
RUN wget -O cdt.deb ${CDT_PKG_URL} \
    && apt-get update \
    && apt-get install -y ./cdt.deb \
    && rm cdt.deb \
    && rm -rf /var/lib/apt/lists/*

# Debug: List CDT installation directory
RUN echo "Listing CDT installation directory: /usr/opt/cdt/" && ls -l /usr/opt/cdt/

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
RUN cd tests && npm install

# Indicate that the container is ready (optional, can be removed)
# CMD ["echo", "Antelope CDT & Node.js environment ready."]
