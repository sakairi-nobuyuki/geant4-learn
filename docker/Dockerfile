FROM ubuntu:22.04

ARG USERNAME=app
ARG UID=1000
ARG GID=1000

RUN apt-get update && \
    apt-get install -y sudo  && \
    groupadd --gid ${GID} ${USERNAME} && \
    useradd -m -u ${UID} -g ${GID} -s /bin/bash ${USERNAME} && \
    echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER ${USERNAME}
WORKDIR /home/${USERNAME}

RUN sudo apt update && \
    sudo apt install -y build-essential cmake ninja-build git wget \
    libexpat1-dev libxerces-c-dev libgl1-mesa-dev libglu1-mesa-dev libx11-dev \
    libxmu-dev libxi-dev qt6-base-dev python3 python3-venv    


WORKDIR /home/${USERNAME}

RUN git clone --branch v11.4.3 --depth 1 https://github.com/Geant4/geant4.git geant4-src    

WORKDIR /home/${USERNAME}/geant4-install
WORKDIR /home/${USERNAME}/geant4-build

RUN cmake -G Ninja \
  -DCMAKE_INSTALL_PREFIX=/home/${USERNAME}/geant4-install \
  -DCMAKE_BUILD_TYPE=Release \
  -DGEANT4_INSTALL_DATA=ON \
  -DGEANT4_USE_QT=ON \
  -DGEANT4_USE_OPENGL_X11=ON \
  ../geant4-src && \
    cmake --build . -j 4 && \
    cmake --install .

