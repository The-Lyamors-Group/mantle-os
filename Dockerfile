FROM debian:bookworm
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends build-essential gcc binutils make xorriso grub-efi-amd64-bin grub-pc-bin grub-common mtools ca-certificates git && update-ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /workspace
ENTRYPOINT ["/bin/sh", "/workspace/build.sh"]
