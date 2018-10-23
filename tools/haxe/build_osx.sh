#!/bin/bash
  set -ev

  git clone --recursive https://github.com/HaxeFoundation/haxe.git ~/haxe --depth 1

  brew update
  brew tap Homebrew/bundle
  brew bundle --file=~/haxe/tests/Brewfile --no-upgrade

  brew install neko --HEAD;

  export OPAMYES=1
  opam init
  eval `opam config env`
  opam update
  opam pin add haxe ~/haxe --no-action
  opam install haxe --deps-only

  # Build haxe
  pushd ~/haxe
  make ADD_REVISION=1 && sudo make install INSTALL_DIR=/usr/local
  popd
  haxe -version
  # setup haxelib
  mkdir ~/haxelib && haxelib setup ~/haxelib
  haxelib dev hxcpp $TRAVIS_BUILD_DIR
  haxelib install record-macros

