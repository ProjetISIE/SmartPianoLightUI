{
  cmake,
  doctest,
  glfw,
  imgui,
  libGL,
  ninja,
  pkg-config,
  SDL2,
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
    # glew # OpenGL Extension Wrangler Library
    glfw
    (imgui.override {
      IMGUI_BUILD_GLFW_BINDING = true;
      IMGUI_BUILD_OPENGL3_BINDING = true;
    })
    libGL # GPU library
    # raylib # Graphics library TEST
    SDL2 # Graphics backend
    # wxwidgets_3_3 # GUI library TEST
  ];
  # cmakeFlags = [
  #   "-DIMGUI_ROOT=${imgui}"
  #   "-DIMGUI_DIR=${imgui}/include/imgui"
  # ];
}
