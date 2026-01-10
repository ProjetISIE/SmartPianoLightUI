{
  cmake,
  doctest,
  libGL,
  ninja,
  pkg-config,
  raylib,
  self,
  stdenv,
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
  ];
}
