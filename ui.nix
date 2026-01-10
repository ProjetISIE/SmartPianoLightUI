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
    # wxwidgets_3_3 # GUI library TEST
    # Linux specific
    wayland
    libdecor
    xorg.libX11
    xorg.libXcursor
    xorg.libXinerama
    xorg.libXi
    xorg.libXrandr
  ];
}
