{
  description = "Dependencies for recovery-web-ui frontend";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs = { self, nixpkgs }:
  let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
  in
  {
    devShells.${system}.default = pkgs.mkShell {
      buildInputs = [
        pkgs.nodejs
        pkgs.prettierd
        pkgs.eslint_d
      ];
      PRETTIERD_LOCAL_PRETTIER_ONLY = true;
      ESLINT_D_LOCAL_ESLINT_ONLY = true;
    };
  };
}
