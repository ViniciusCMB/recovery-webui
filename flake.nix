{
  description = "Dependencies for recovery-web-ui backend";

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
      buildInputs = with pkgs; [
        (python3.withPackages (p: with p; [
          virtualenv
        ]))
      ];

      shellHook = ''
        if [ ! -d "src/.venv" ]; then
          python -m venv "src/.venv"
        fi
        source src/.venv/bin/activate
        pip install -r requirements.txt
      '';
    };
  };
}
