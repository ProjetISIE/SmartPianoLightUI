{
  cmake,
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
}:
stdenv.mkDerivation rec {
  pname = "smart-piano-ui";
  version = "0.1.0";
  src = lib.cleanSource ./.;
  nativeBuildInputs = [
    # clang # C/C++ compiler
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
  ++ lib.optionals stdenv.isLinux [
    wayland
    libGL # GPU library (null in nixpkgs on Darwin)
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
