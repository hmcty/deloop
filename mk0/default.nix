{ pkgs ? import (fetchTarball "https://github.com/NixOS/nixpkgs/archive/8b27c1239e5c421a2bbc2c65d52e4a6fbf2ff296.tar.gz") {} }:

let
  version = "0.1.0";
  fs = pkgs.lib.fileset;
  firmware_fileset = fs.unions [
    ./CMakeLists.txt
    ./scripts/create_log_table.py
    ./cmake
    ./external
    ./proto
    ./python/deloop_mk0/log_table.json
    (fs.fileFilter
      (file: file.hasExt "c"
              || file.hasExt "cpp"
              || file.hasExt "h"
              || file.hasExt "hpp"
              || file.hasExt "ld"
              || file.hasExt "s")
      ./src)
  ];

  mkDeloopDerivation = {
    name,
    fileset,
    mVersion ? version,
    mBuildPhase ? null,
    mCheckPhase ? null,
    mInstallPhase ? null,
    cmakeExtraFlags ? [],
    extraBuildInputs ? [],
  } :
    pkgs.stdenv.mkDerivation rec {
      pname = name;
      version = mVersion;
      src = fs.toSource {
        root = ./.;
        fileset = fileset;
      };

      buildInputs = [
        pkgs.cmake
        pkgs.protobuf_25
        pkgs.python3
        pkgs.python3Packages.protobuf
        pkgs.python3Packages.grpcio-tools
      ] ++ extraBuildInputs;

      cmakeFlags = [
        "-DPROTOBUF_PROTOC_EXECUTABLE:FILEPATH=${pkgs.protobuf_25}/bin/protoc"
      ] ++ cmakeExtraFlags;

      postPatch = ''
        chmod +x external/nanopb/generator/protoc-gen-nanopb
        patchShebangs external/nanopb/generator/protoc-gen-nanopb

        chmod +x scripts/create_log_table.py
        patchShebangs scripts/create_log_table.py
      '';

      buildPhase = if mBuildPhase != null then mBuildPhase else "make";
      doCheck = mCheckPhase != null;
      checkPhase = if mCheckPhase != null then mCheckPhase else "";
      installPhase = if mInstallPhase != null then mInstallPhase else "";
    };

  firmware = mkDeloopDerivation {
    name = "deloop-mk0-firmware";
    fileset = firmware_fileset;
    cmakeExtraFlags = [
      "-DCMAKE_TOOLCHAIN_FILE:PATH=cmake/arm-none-eabi-gcc.cmake"
    ];
    extraBuildInputs = [pkgs.gcc-arm-embedded-13];
    mInstallPhase = ''
      mkdir -p $out/bin $out/python
      cp *.hex *.elf *.bin *.map *.lst *.size $out/bin
      cp python/*_pb2.py python/log_table.json $out/python
    '';
  };

  tests = mkDeloopDerivation {
    name = "deloop-mk0-tests";
    fileset = fs.unions [ firmware_fileset ./tests ];
    cmakeExtraFlags = [
      "-DENABLE_TESTING=ON"
      "-DMCU_TARGET=HOST"
    ];
    mCheckPhase = "ctest --output-on-failure";
    mBuildPhase = "make all_tests";  # TODO: Fix `make all` on host.
    mInstallPhase = ''
      mkdir -p $out/bin
      find tests -type f -executable -name "test_*" -exec cp {} $out/bin \;
    '';
  };

  sdk = pkgs.python3Packages.buildPythonApplication {
    pname = "deloop-mk0-sdk";
    version = "0.1.0";
    src = fs.toSource {
      root = ./python;
      fileset = fs.unions [
        (fs.fileFilter (file: file.hasExt "py") ./python/.)
      ];
    };

    buildInputs = [ firmware ];

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
      cp -r ${firmware}/python/*_pb2.py ./
      cp -r ${firmware}/python/log_table.json ./deloop_mk0
    '';

    doCheck = false;
  };

in
{
  default = pkgs.stdenv.mkDerivation rec {
    pname = "deloop-mk0";
    version = "0.1.0";

    phases = [ "installPhase" ];
    installPhase = ''
      mkdir -p $out/bin
      cp -r ${firmware}/bin/* $out/bin
      cp -r ${sdk}/* $out
    '';
  };
  firmware = firmware;
  tests = tests;
  sdk = sdk;
}
