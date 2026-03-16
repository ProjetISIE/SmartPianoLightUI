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
      forSystems = nixpkgs.lib.genAttrs [
        "x86_64-linux" # "aarch64-linux"
        "aarch64-darwin"
      ];
    in
    {
      packages = forSystems (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
          mkUi =
            p:
            p.callPackage ./ui.nix {
              inherit self;
              inherit (p) glfw;
              engine =
                engine.packages.${pkgs.stdenv.hostPlatform.system}.${
                  if pkgs.stdenv.hostPlatform.system == p.stdenv.hostPlatform.system then
                    "default"
                  else
                    "${p.stdenv.hostPlatform.system}-default"
                };
              stdenv = p.clangStdenv;
            };
        in
        {
          default = mkUi pkgs;
          smart-piano = mkUi pkgs;
        }
        // nixpkgs.lib.optionalAttrs (system == "x86_64-linux") {
          aarch64-linux-default = mkUi pkgs.pkgsCross.aarch64-multiplatform;
          aarch64-linux-smart-piano = mkUi pkgs.pkgsCross.aarch64-multiplatform;
        }
      );
      devShells = forSystems (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
          defaultPkg = self.packages.${system}.default;
        in
        {
          default =
            pkgs.mkShell.override
              {
                stdenv = pkgs.clangStdenv; # Clang instead of GCC
              }
              {
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
                inputsFrom = [ defaultPkg ];
                shellHook = ''
                  # export XDG_SESSION_TYPE=wayland
                  # export SDL_VIDEODRIVER=wayland 
                  # export _JAVA_AWT_WM_NONREPARENTING=1
                  export LD_LIBRARY_PATH="${pkgs.lib.makeLibraryPath defaultPkg.buildInputs}:$LD_LIBRARY_PATH"
                  cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Debug \
                    -DCOVERAGE=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
                '';
              };
        }
      );
    };
}
