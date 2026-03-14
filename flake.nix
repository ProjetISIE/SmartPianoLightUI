{
  description = "Nix flake C++23 cross development environment";
  nixConfig = {
    extra-substituters = [ "https://cache.garnix.io" ];
    extra-trusted-public-keys = [ "cache.garnix.io:CTFPyKSLcx5RMJKfLo5EEPUObbA78b0YQ2DTCJXqr9g=" ];
  };
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
      localSystems = [
        "x86_64-linux" # "aarch64-linux"
        "aarch64-darwin"
      ];
      crossSystem = "aarch64-linux";
      forSystems = f: nixpkgs.lib.genAttrs localSystems (system: f (import nixpkgs { inherit system; }));
      forCross =
        f:
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
      packages = forSystems (pkgs: rec {
        smart-piano = pkgs.callPackage ./ui.nix {
          inherit self pkgs;
          inherit (pkgs) glfw;
          engine = engine.packages.${pkgs.stdenv.hostPlatform.system}.default;
          stdenv = pkgs.clangStdenv;
        };
        default = smart-piano;
      });
      crossPackages = forCross (
        pkgs: crossPkgs: rec {
          smart-piano = crossPkgs.callPackage ./ui.nix {
            inherit self;
            inherit (crossPkgs) glfw;
            engine = engine.packages.${crossPkgs.stdenv.hostPlatform.system}.default;
            pkgs = crossPkgs;
            stdenv = crossPkgs.clangStdenv;
          };
          default = smart-piano;
        }
      );
      devShells = forSystems (pkgs: {
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
      });
    };
}
