{
  description = "FreeCAD develop flake";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    systems.url = "github:nix-systems/default";
    flake-utils = {
      url = "github:numtide/flake-utils";
      inputs.systems.follows = "systems";
    };
  };

  outputs =
    { nixpkgs, flake-utils, ... }@inputs:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        # run `nix build "./package/nixos?submodules=1"` in top-level to build freecad with nixpkgs
        packages = rec {
          freecad = pkgs.callPackage ./freecad.nix { };
          default = freecad;
        };
      }
    );
}
