FROM debian:bookworm
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends build-essential bc bison flex perl python3 autoconf automake libtool gawk cmake ninja-build libssl-dev libelf-dev cpio xz-utils bzip2 curl xorriso grub-efi-amd64-bin grub-pc-bin grub-common mtools e2fsprogs util-linux imagemagick ca-certificates git && rm -rf /var/lib/apt/lists/*
WORKDIR /workspace
ENTRYPOINT ["/bin/sh", "/workspace/build.sh"]
