{
  cmake,
  doctest,
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
  stdenv,
  wayland,
  xorg,
}:
stdenv.mkDerivation rec {
  pname = "smart-piano-ui";
  version = "0.0.0";
  src = self;
  src = lib.cleanSource ./.;
  nativeBuildInputs = [
    # clang # C/C++ compiler
    cmake # Modern build tool
    llvm # For llvm-cov
    ninja # Modern build tool
    pkg-config # Build tool
  ];
  buildInputs = [
    doctest # Testing framework
    engine # SmartPianoEngine
    glfw # Raylib dependency
    libGL # GPU library
    raylib # Graphics library
  ]
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
    cmakeFlagsArray+=("-DENGINE_PATH=${engine}")
  '';
  installPhase = ''
    runHook preInstall
    mkdir --parents --verbose $out/bin
    cp --verbose src/main $out/bin/${pname}
    runHook postInstall
  '';
  meta = with lib; {
    description = "Smart Piano User Interface";
    license = licenses.gpl3Plus;
    platforms = platforms.all;
  };
}
