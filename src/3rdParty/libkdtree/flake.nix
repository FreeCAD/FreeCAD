{
  description = "Nix flake for libkdtree+ library";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    flake-compat.url = "https://flakehub.com/f/edolstra/flake-compat/1.tar.gz";
  };

  outputs = { self, nixpkgs, ... }@inputs: let
    lib = nixpkgs.lib;
    shellSystems = nixpkgs.lib.systems.flakeExposed;
    buildSystems = nixpkgs.lib.systems.flakeExposed;
    forSystems = systems: f: nixpkgs.lib.genAttrs systems (system: f system);
  in {

    devShells = forSystems shellSystems (system: let
      pkgs = nixpkgs.legacyPackages.${system};
    in {
      default = pkgs.mkShell {
        nativeBuildInputs = with pkgs; [
          nil # lsp language server for nix
          cmake
          doxygen
        ] ++ lib.optionals pkgs.hostPlatform.isLinux [ gdb ];
      };
    });

    packages = forSystems buildSystems (system: let
      pkgs = nixpkgs.legacyPackages.${system}.pkgs;
    in {
      default = self.packages.${system}.libkdtree;
      libkdtree = pkgs.callPackage ./libkdtree.nix {};
    });

  };
}