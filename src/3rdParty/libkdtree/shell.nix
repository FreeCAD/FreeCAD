# Nix development environment for libkdtree++
{
  pkgs ? import <nixpkgs> {}
}:

pkgs.mkShell rec {
   buildInputs = with pkgs; [
    cmake
    doxygen

    # debugging tools
    gdb
   ];
}