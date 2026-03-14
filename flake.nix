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
            "x86_64-linux" # "aarch64-linux"
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
            inherit self pkgs;
            inherit (pkgs) glfw;
            engine = engine.packages.${pkgs.stdenv.hostPlatform.system}.default;
            stdenv = pkgs.clangStdenv;
          };
          cross-smart-piano = crossPkgs.callPackage ./ui.nix {
            inherit self;
            inherit (crossPkgs) glfw;
            engine =
              engine.packages.${pkgs.stdenv.hostPlatform.system}."${crossPkgs.stdenv.hostPlatform.system}-default";
            pkgs = crossPkgs;
            stdenv = crossPkgs.clangStdenv;
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
                packages =
                  with pkgs;
                  [
                    bashInteractive
                    clang-tools # Clang CLIs, including LSP
                    cmake-format # CMake formatter
                    cmake-language-server # Cmake LSP
                    doxygen # Documentation generator
                    lldb # Clang debug adapter
                  ]
                  ++ lib.optionals stdenv.isLinux [
                    alsa-utils # aconnect…
                    clang-uml # UML diagram generator
                    cppcheck # C++ Static analysis
                    fluidsynth # JACK Synthesizer
                    qsynth # FluidSynth GUI
                    socat # Serial terminal for manual testing
                    valgrind # Debugging and profiling
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
