{
  description = "Nix flake C++ development environment";
  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  outputs =
    { self, nixpkgs }:
    let
      systems =
        f:
        let
          localSystems = [
            "aarch64-linux"
            "x86_64-linux"
            "aarch64-darwin"
          ];
          crossSystem = "aarch64-linux";
        in
        nixpkgs.lib.genAttrs localSystems (
          localSystem:
          f (import nixpkgs { inherit localSystem; }) (
            import nixpkgs {
              inherit localSystem;
              inherit crossSystem;
            }
          )
        );
    in
    {
      packages = systems (
        pkgs: crossPkgs: {
          ui = pkgs.callPackage ./ui.nix { inherit self; };
          cross-ui = crossPkgs.callPackage ./ui.nix { inherit self; };
          default = self.packages.${pkgs.stdenv.hostPlatform.system}.ui;
          cross = self.packages.${pkgs.stdenv.hostPlatform.system}.cross-ui;
        }
      );
      devShells = systems (
        pkgs: crossPkgs: {
          default =
            pkgs.mkShell.override
              {
                stdenv = pkgs.clangStdenv; # Clang instead of GCC
              }
              {
                packages = with pkgs; [
                  clang-tools # Clang CLIs, including LSP
                  # clang-uml # UML diagram generator
                  # cmake-format # CMake formatter
                  cmake-language-server # Cmake LSP
                  # cppcheck # C++ Static analysis
                  doxygen # Documentation generator
                  # gtest # Testing framework
                  imgui # GUI library TEST
                  # lcov # Code coverage
                  lldb # Clang debug adapter
                  neocmakelsp # CMake LSP
                  raylib # Graphics library TEST
                  # valgrind # Debugging and profiling
                  wxwidgets_3_3 # GUI library TEST
                ];
                nativeBuildInputs = self.packages.${pkgs.stdenv.hostPlatform.system}.ui.nativeBuildInputs;
                buildInputs = self.packages.${pkgs.stdenv.hostPlatform.system}.ui.buildInputs;
                # Export compile commands JSON for LSP and other tools
                shellHook = ''
                  mkdir --verbose build
                  cd build
                  cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
                '';
              };
        }
      );
    };
}
