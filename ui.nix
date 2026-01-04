{
  cmake,
  doctest,
  ninja,
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
    # pkg-config # Build tool
  ];
  # buildInputs = [
  # ];
}
