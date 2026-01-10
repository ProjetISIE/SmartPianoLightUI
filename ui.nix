{
  cmake,
  doctest,
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
    cmake # Modern build tool
    doctest # Testing framework
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
}
