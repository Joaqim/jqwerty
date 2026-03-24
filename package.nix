{
  lib,
  stdenv,
  cmake,
  extra-cmake-modules,
  fcitx5,
  libxkbcommon,
}:

stdenv.mkDerivation {
  pname = "fcitx5-jqwerty";
  version = "0.1.1";

  src = lib.cleanSource ./.;

  nativeBuildInputs = [
    cmake
    extra-cmake-modules
  ];

  buildInputs = [
    fcitx5
    libxkbcommon
  ];

  meta = {
    description = "fcitx5 jqwerty engine — physical-position key remapping";
    license = lib.licenses.mit;
    platforms = lib.platforms.linux;
  };
}
