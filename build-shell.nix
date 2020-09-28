with (import <nixpkgs> {});
mkShell {
  buildInputs = [
    alsaLib
    clang_11
    cmake
    fftw
    fftwFloat
    fftwLongDouble
    fmt
    glog
    gmock
    gperftools
    libv4l
    # llvm_11
    llvmPackages_11.lldClang.bintools 
    ncurses
    ninja
  ];
}
