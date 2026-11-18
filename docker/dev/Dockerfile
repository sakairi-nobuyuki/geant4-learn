FROM geant4-base AS geant4-dev


# Codex CLI
RUN sudo apt-get update \
    && sudo apt-get install -y --no-install-recommends \
        git curl ca-certificates gnupg \
    && sudo apt-get remove -y nodejs npm \
    && curl -fsSL https://deb.nodesource.com/setup_22.x | sudo -E bash - \
    && sudo apt-get install -y --no-install-recommends nodejs 


# Codex CLI
RUN sudo npm install -g @openai/codex

