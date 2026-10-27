{
    description = "CMake development environment";

    inputs = {
        nixpkgs.url = "nixpkgs";
        flake-utils.url = "github:numtide/flake-utils";
    };

    outputs = { self, nixpkgs, flake-utils }:
        flake-utils.lib.eachDefaultSystem (system:
            let
                pkgs = nixpkgs.legacyPackages.${system};
            in {
                devShells.default = pkgs.mkShell rec {
                    nativeBuildInputs = [ pkgs.pkg-config ];
                    buildInputs = with pkgs; [
                        clang
                        cmake

                        libx11
                        libxrandr
                        libxinerama
                        libxcursor
                        libxi
                        libGL
                        xorgproto
                    ];
                };
            }
    );
}
