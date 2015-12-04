#!/bin/bash

pkgname=cyclus
upversion=develop
version=1.4.0
descrip="The nuclear fuel cycle simulator."
maintainer="Robert Carlsen <rwcarlsen@gmail.com>"

fname=${pkgname}_${version}.orig.tar.gz
curl -L https://github.com/cyclus/cyclus/archive/${upversion}.tar.gz > $fname
tar -xf $fname
mv $pkgname-$upversion $pkgname-$version
cd $pkgname-$version
mkdir -p debian/source

echo "9" > debian/compat
touch debian/copyright
echo "3.0 (quilt)" > debian/source/format

echo "
Source: $pkgname
Maintainer: $maintainer
Section: misc
Priority: optional
Standards-Version: 3.9.2
Build-Depends: debhelper (>= 9), cmake

Package: $pkgname
Architecture: any
Depends: "'${shlibs:Depends}, ${misc:Depends}, coinor-libcbc, coinor-libclp, coinor-libcoinutils, coinor-libosi, libhdf5-serial, python'"
Description: $descrip
" > debian/control

echo '#!/usr/bin/make -f
%:
	dh $@

' > debian/rules

echo "$pkgname ($version-1) UNRELEASED; urgency=low

  * blablabla
  * This is my first Debian package.
  * initial packaged version...

 -- $maintainer  $(date -R)
" > debian/changelog

debuild -us -uc
