{
  description = "A simple Tiny BASIC compiler written in C";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
		flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }: flake-utils.lib.eachDefaultSystem (system: let
		pkgs = import nixpkgs { inherit system; };

		pkg-name = "tinybc";
		build-deps = [ pkgs.clang ];
	in {
		defaultPackage = pkgs.stdenv.mkDerivation {
			name = pkg-name;
			src = ./.;

			nativeBuildInputs = build-deps;

			buildPhase = ''
				make build --always-make CC_ARGS=-O3 BUILD_DIR=./
			'';

			installPhase = ''
				mkdir -p $out/bin
				cp ${pkg-name} $out/bin
			'';
		};

		devShell = pkgs.mkShell {
			buildInputs = build-deps ++ [ pkgs.valgrind ];
		};
	});
}
