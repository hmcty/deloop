{ pkgs ? import (fetchTarball "https://github.com/NixOS/nixpkgs/archive/8b27c1239e5c421a2bbc2c65d52e4a6fbf2ff296.tar.gz") {} }:

let deloop-mk0 = import ./default.nix { inherit pkgs; };

in pkgs.mkShell {
  buildInputs = [
    deloop-mk0
    pkgs.gcc-arm-embedded-13
    pkgs.stlink
  ];

  shellHook = ''
    export DELOOP_MK0_PATH=${deloop-mk0}
  '';
}
