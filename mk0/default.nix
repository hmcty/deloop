{ pkgs ? import (fetchTarball "https://github.com/NixOS/nixpkgs/archive/8b27c1239e5c421a2bbc2c65d52e4a6fbf2ff296.tar.gz") {} }:

let
  mk0-dev = pkgs.stdenv.mkDerivation rec {
    pname = "deloop-mk0-dev";
    version = "0.1.0";
    src = ./.;

    buildInputs = [
      pkgs.cmake
      pkgs.gcc-arm-embedded-13
      pkgs.protobuf_25
      pkgs.python3
      pkgs.python3Packages.protobuf
      pkgs.python3Packages.grpcio-tools
    ];

    postPatch = ''
      chmod +x external/nanopb/generator/protoc-gen-nanopb
      patchShebangs external/nanopb/generator/protoc-gen-nanopb

      chmod +x scripts/create_log_table.py
      patchShebangs scripts/create_log_table.py
    '';

    cmakeFlags = [
      "-DCMAKE_TOOLCHAIN_FILE:PATH=cmake/arm-none-eabi-gcc.cmake"
      "-DCMAKE_BUILD_TYPE=Debug"
      "-DPROTOBUF_PROTOC_EXECUTABLE:FILEPATH=${pkgs.protobuf_25}/bin/protoc"
    ];

    buildPhase = ''
      make
    '';

    installPhase = ''
      mkdir -p $out/bin $out/python
      cp *.hex *.elf *.bin *.map *.lst *.size $out/bin
      cp ../python/*_pb2.py ../python/*.json $out/python
    '';
  };

  mk0-app = pkgs.python3Packages.buildPythonApplication {
    pname = "deloop-mk0-app";
    version = "0.1.0";
    src = pkgs.lib.cleanSource ./python;

    buildInputs = [ mk0-dev ];

    build-system = with pkgs.python3Packages; [
      setuptools
    ];

    dependencies = with pkgs.python3Packages; [
      protobuf
      grpcio-tools
      cmd2
      tabulate
      pyserial
    ];

    postPatch = ''
      cp -r ${mk0-dev}/python/*_pb2.py ./
      cp -r ${mk0-dev}/python/*.json ./deloop_mk0
    '';

    doCheck = false;
  };
in
pkgs.stdenv.mkDerivation rec {
  pname = "deloop-mk0";
  version = "0.1.0";

  phases = [ "installPhase" ];
  installPhase = ''
    mkdir -p $out/bin
    cp -r ${mk0-dev}/bin/* $out/bin
    cp -r ${mk0-app}/* $out
  '';
}
