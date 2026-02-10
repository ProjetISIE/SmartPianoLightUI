{
  description = "Nix flake C++23 development environment";
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    engine.url = "github:ProjetISIE/SmartPianoEngine";
  };
  outputs =
    {
      self,
      nixpkgs,
      engine,
    }:
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
        pkgs: crossPkgs: rec {
          smart-piano = pkgs.callPackage ./ui.nix {
            inherit self;
            stdenv = pkgs.clangStdenv;
            engine = engine.packages.${pkgs.stdenv.hostPlatform.system}.default;
          };
          cross-smart-piano = crossPkgs.callPackage ./ui.nix {
            inherit self;
            stdenv = crossPkgs.clangStdenv;
            engine = engine.packages.${crossPkgs.stdenv.hostPlatform.system}.default;
          };
          default = smart-piano;
          cross = cross-smart-piano;
        }
      );
      devShells = systems (
        pkgs: crossPkgs: {
          default =
            pkgs.mkShell.override
              {
                stdenv = pkgs.clangStdenv; # Clang instead of GCC
              }
              rec {
                packages = with pkgs; [
                  clang-tools # Clang CLIs, including LSP
                  clang-uml # UML diagram generator
                  cmake-format # CMake formatter
                  cmake-language-server # Cmake LSP
                  # cppcheck # C++ Static analysis
                  doxygen # Documentation generator
                  # fluidsynth # JACK Synthesizer
                  lldb # Clang debug adapter
                  # qsynth # FluidSynth GUI
                  # socat # Serial terminal for manual testing
                  # valgrind # Debugging and profiling
                ];
                nativeBuildInputs = self.packages.${pkgs.stdenv.hostPlatform.system}.smart-piano.nativeBuildInputs;
                buildInputs = self.packages.${pkgs.stdenv.hostPlatform.system}.smart-piano.buildInputs;
                # Export compile commands JSON for LSP and other tools
                shellHook = ''
                  export LD_LIBRARY_PATH="${pkgs.lib.makeLibraryPath buildInputs}:$LD_LIBRARY_PATH"
                  # export XDG_SESSION_TYPE=wayland
                  # export SDL_VIDEODRIVER=wayland 
                  # export _JAVA_AWT_WM_NONREPARENTING=1
                  mkdir --verbose build
                  cd build
                  cmake -DCMAKE_BUILD_TYPE=Debug -DCOVERAGE=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
                '';
              };
        }
      );
    };
}
