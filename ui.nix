{
  cmake,
  clang,
  doctest,
  llvm,
  libdecor,
  libGL,
  ninja,
  pkg-config,
  raylib,
  self,
  stdenv,
  wayland,
  xorg,
}:
stdenv.mkDerivation {
  pname = "ui";
  version = "0.0.0";
  src = self;
  # doCheck = true; # Enable tests
  nativeBuildInputs = [
    clang # C/C++ compiler
    cmake # Modern build tool
    doctest # Testing framework
    llvm # For llvm-cov
    ninja # Modern build tool
    pkg-config # Build tool
  ];
  buildInputs = [
    libGL # GPU library
    raylib # Graphics library
    # Linux specific TODO don’t include on other platforms
    wayland
    libdecor
    xorg.libX11
    xorg.libXcursor
    xorg.libXinerama
    xorg.libXi
    xorg.libXrandr
  ];
  installPhase = ''
    mkdir --parents --verbose $out/bin
    cp --verbose src/main $out/bin/ui
  '';
}
