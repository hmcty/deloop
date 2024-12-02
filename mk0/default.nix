{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation rec {
  pname = "deloop-mk0";
  version = "0.1.0";

  src = ./.;

  buildInputs = [
    pkgs.cmake
    pkgs.gcc-arm-embedded-13
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
