{ pkgs ? import (fetchTarball "https://github.com/NixOS/nixpkgs/archive/8b27c1239e5c421a2bbc2c65d52e4a6fbf2ff296.tar.gz") {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    stlink
    gcc-arm-embedded-13
    protobuf_25
    python3
    python3Packages.protobuf
    python3Packages.grpcio-tools
  ];
}
