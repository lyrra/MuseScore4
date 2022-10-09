#!/bin/sh

# install guile

F=mingw-w64-x86_64-guile3-3.0.5-1-any.pkg.tar.zst

wget https://github.com/lyrra/guile/releases/download/weekly-mingw/$F
pacman -U --noconfirm $F

echo "--------- fix /mingw64/lib/pkgconfig/guile-3.0.pc  ------------"
cat /mingw64/lib/pkgconfig/guile-3.0.pc

tee /mingw64/lib/pkgconfig/guile-3.0.pc << 'EOF'
prefix=/mingw64
exec_prefix=/mingw64
bindir=${prefix}/bin
libdir=${prefix}/lib
includedir=${prefix}/include
datarootdir=${prefix}/share
datadir=${prefix}/share
pkgdatadir=${prefix}/share/guile
pkgincludedir=${prefix}/include/guile

sitedir=${prefix}/share/guile/site/3.0
extensiondir=${prefix}/lib/guile/3.0/extensions
siteccachedir=${prefix}/lib/guile/3.0/site-ccache
libguileinterface=4:0:3

# Actual name of the 'guile' and 'guild' programs.  This is
# particularly useful when '--program-transform-name' or similar has
# been used.
guild=${bindir}/guild
guile=${bindir}/guile

Name: GNU Guile
Description: GNU's Ubiquitous Intelligent Language for Extension
Version: UNKNOWN
Libs: -L${libdir} -lguile-3.0 ${prefix}/lib/libgc.a -lgc
Libs.private:    -L${prefix}/lib -lffi   \
  ${prefix}/lib/libunistring.dll.a -lws2_32  -lm ${prefix}/lib/libiconv.dll.a ${prefix}/lib/libintl.dll.a
Cflags: -I${pkgincludedir}/3.0 -I${prefix}/include

EOF

echo "--------- new: /mingw64/lib/pkgconfig/guile-3.0.pc  ------------"
cat /mingw64/lib/pkgconfig/guile-3.0.pc
