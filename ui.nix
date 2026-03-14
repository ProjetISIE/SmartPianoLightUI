{
  cmake,
  clang,
  engine,
  glfw,
  llvm,
  lib,
  libdecor,
  libGL,
  libx11,
  libxcursor,
  libxinerama,
  libxi,
  libxrandr,
  ninja,
  pkg-config,
  raylib,
  self,
  stdenv,
  wayland,
}:
stdenv.mkDerivation {
  pname = "ui";
  version = "0.0.0";
  src = self;
  nativeBuildInputs = [
    clang # C/C++ compiler
    cmake # Modern build tool
    llvm # For llvm-cov
    ninja # Modern build tool
    pkg-config # Build tool
  ];
  buildInputs = [
    engine # SmartPianoEngine
    glfw # Raylib dependency
    raylib # Graphics library
  ]
  ++ lib.optional (!stdenv.isDarwin) libGL # GPU library (null in nixpkgs on Darwin)
  ++ lib.optionals stdenv.isLinux [
    wayland
    libdecor
    libx11
    libxcursor
    libxinerama
    libxi
    libxrandr
  ];
  preConfigure = ''
    cmakeFlagsArray+=("-DENGINE_PATH=${engine}" "-DCOVERAGE=OFF")
  '';
  installPhase = ''
    mkdir --parents --verbose $out/bin
    cp --verbose src/main $out/bin/ui
  '';
}
