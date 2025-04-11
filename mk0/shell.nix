{ pkgs ? import (fetchTarball "https://github.com/NixOS/nixpkgs/archive/8b27c1239e5c421a2bbc2c65d52e4a6fbf2ff296.tar.gz") {} }:

pkgs.mkShell {
  buildInputs = [
    pkgs.cmake
    pkgs.gcc-arm-embedded-13
    pkgs.stlink
    pkgs.protobuf_25
    pkgs.python3
    pkgs.python3Packages.protobuf
    pkgs.python3Packages.grpcio-tools
  ];
}
