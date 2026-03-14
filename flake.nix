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
      supportedSystems = [
        "x86_64-linux" # "aarch64-linux"
        "aarch64-darwin" # "x86_64-darwin"
      ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
    in
    {
      packages = forAllSystems (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
          makeSmartPiano =
            p:
            p.callPackage ./ui.nix {
              stdenv = p.clangStdenv;
              engine =
                engine.packages.${p.stdenv.hostPlatform.system}.default or engine.packages.${system}.default;
            };
        in
        {
          default = makeSmartPiano pkgs;
          smart-piano = makeSmartPiano pkgs;
        }
        // nixpkgs.lib.optionalAttrs (system == "x86_64-linux") {
          smart-piano-aarch64 = (pkgs.pkgsCross.aarch64-multiplatform).callPackage ./ui.nix {
            stdenv = (pkgs.pkgsCross.aarch64-multiplatform).clangStdenv;
            engine = engine.packages.${system}.smart-piano-engine-aarch64 or engine.packages.${system}.default;
          };
        }
      );
      devShells = forAllSystems (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
          # We use the native package to get build inputs
          smart-piano = self.packages.${system}.default;
        in
        {
          default = pkgs.mkShell {
            inputsFrom = [ smart-piano ];
            packages = with pkgs; [
              clang-tools # Clang CLIs, including LSP
              clang-uml # UML diagram generator
              cmake-format # CMake formatter
              cmake-language-server # Cmake LSP
              cmake-lint
              # cppcheck # C++ Static analysis
              doxygen # Documentation generator
              # fluidsynth # JACK Synthesizer
              lldb # Clang debug adapter
              # qsynth # FluidSynth GUI
              # socat # Serial terminal for manual testing
              # valgrind # Debugging and profiling
            ];
            shellHook = ''
              export LD_LIBRARY_PATH="${pkgs.lib.makeLibraryPath smart-piano.buildInputs}:$LD_LIBRARY_PATH"
              cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Debug \
                -DCOVERAGE=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
            '';
          };
        }
      );
      checks = forAllSystems (system: self.packages.${system});
    };
}
