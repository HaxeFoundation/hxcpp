#!/bin/bash
  set -ev

  brew update --merge
  # Install haxe dependencies
  brew uninstall --force brew-cask # https://github.com/caskroom/homebrew-cask/pull/15381
  brew tap Homebrew/bundle
  brew install opam
  export OPAMYES=1
  opam init
  opam list -a
  opam init --comp 4.04.2
  opam install camlp4 sedlex ocamlfind camlzip xml-light extlib rope ptmap
  eval `opam config env`

  brew install neko --HEAD;

  # Build haxe
  git clone --recursive https://github.com/HaxeFoundation/haxe.git ~/haxe --depth 1
  pushd ~/haxe
  make ADD_REVISION=1 && sudo make install INSTALL_DIR=/usr/local
  popd
  haxe -version
  # setup haxelib
  mkdir ~/haxelib && haxelib setup ~/haxelib
  haxelib dev hxcpp $TRAVIS_BUILD_DIR
  haxelib install record-macros

