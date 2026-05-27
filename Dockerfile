FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
ARG PIXI_VERSION=v0.48.2

ENV PIXI_HOME=/opt/pixi \
    PATH=/opt/pixi/bin:$PATH \
    QT_QPA_PLATFORM=offscreen \
    FREECAD_USER_HOME=/workspace/.freecad

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        dbus-x11 \
        git \
        novnc \
        openbox \
        tini \
        websockify \
        xterm \
        x11vnc \
        xvfb \
    && rm -rf /var/lib/apt/lists/*

RUN curl -fsSL https://pixi.sh/install.sh | bash -s -- -y --version "${PIXI_VERSION}" \
    && pixi --version

WORKDIR /workspace/FreeCAD

COPY pixi.toml pixi.lock ./
RUN pixi install --locked

RUN git config --global --add safe.directory /workspace/FreeCAD

COPY . .

COPY docker/start-novnc.sh /usr/local/bin/start-novnc
RUN chmod +x /usr/local/bin/start-novnc

ENTRYPOINT ["/usr/bin/tini", "--"]
CMD ["bash"]
