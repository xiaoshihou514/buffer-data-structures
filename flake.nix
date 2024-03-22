{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };
  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem
      (system:
        let
          pkgs = import nixpkgs {
            inherit system;
          };
        in
        with pkgs;
        {
          devShells.default = mkShell {
            buildInputs = [
              # -std=c2x
              pkgs.gcc13
              # for clangd 17, which has -std=c2x support
              pkgs.clang-tools_17

              # libraries
              # unit test framework
              criterion
            ];
            shellHook = ''
              export PATH=$PATH:${pkgs.clang-tools}/bin
              exec fish
            '';
          };
        }
      );
}
