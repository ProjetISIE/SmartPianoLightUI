{
  cmake,
  clang,
  doctest,
  engine,
  glfw,
  llvm,
  libGL,
  ninja,
  pkg-config,
  pkgs,
  raylib,
  self,
  stdenv,
}:
stdenv.mkDerivation {
  pname = "ui";
  version = "0.0.0";
  src = self;
  # doCheck = true; # Enable tests TODO
  nativeBuildInputs = [
    clang # C/C++ compiler
    cmake # Modern build tool
    doctest # Testing framework
    llvm # For llvm-cov
    ninja # Modern build tool
    pkg-config # Build tool
  ];
  buildInputs = [
    engine # SmartPianoEngine
    glfw # Raylib dependency
    libGL # GPU library
    raylib # Graphics library
  ]
  ++ pkgs.lib.optionals stdenv.isLinux [
    pkgs.wayland
    pkgs.libdecor
    pkgs.xorg.libX11
    pkgs.xorg.libXcursor
    pkgs.xorg.libXinerama
    pkgs.xorg.libXi
    pkgs.xorg.libXrandr
  ];

  preConfigure = ''
    cmakeFlagsArray+=("-DENGINE_PATH=${engine}")
  '';

  installPhase = ''
    mkdir --parents --verbose $out/bin
    cp --verbose src/main $out/bin/ui
  '';
}
