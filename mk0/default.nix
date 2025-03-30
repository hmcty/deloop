{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation rec {
  pname = "deloop-mk0";
  version = "0.1.0";

  src = ./.;

  buildInputs = [
    pkgs.cmake
    pkgs.gcc-arm-embedded-13
    pkgs.protobuf_25
    pkgs.python3
    pkgs.python3Packages.protobuf
    pkgs.python3Packages.grpcio-tools
    pkgs.python3Packages.pyserial
    pkgs.python3Packages.ipython
    pkgs.python3Packages.tabulate
  ];

  configurePhase = ''
    cmake -DCMAKE_TOOLCHAIN_FILE:PATH="cmake/arm-none-eabi-gcc.cmake" \
     -DCMAKE_BUILD_TYPE=Debug \
     .
  '';

  buildPhase = ''
    make
  '';

  installPhase = ''
    mkdir -p $out/bin
    cp *.hex *.elf *.bin *.map *.lst *.size $out/bin
  '';
}
