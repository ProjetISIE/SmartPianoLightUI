{
  lib,
  stdenv,
  cmake,
  ninja,
  pkg-config,
  raylib,
  glfw,
  libGL,
  doctest,
  engine,
  llvm,
  wayland,
  libdecor,
  xorg,
}:
stdenv.mkDerivation rec {
  pname = "smart-piano-ui";
  version = "0.1.0";
  src = lib.cleanSource ./.;
  nativeBuildInputs = [
    # clang # C/C++ compiler
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
  ++ lib.optionals stdenv.isLinux [
    wayland
    libdecor
    xorg.libX11
    xorg.libXcursor
    xorg.libXinerama
    xorg.libXi
    xorg.libXrandr
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
