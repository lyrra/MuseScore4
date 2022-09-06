
#!/bin/sh

# install guile

wget https://github.com/lyrra/guile/suites/8151175717/artifacts/353802284

ls -ltr

unzip -a mingw-w64-x86_64-guile3-3-1-any.zip

ls -ltr

F=`pwd`/mingw-w64-x86_64-guile3-3.0.5-1-any.pkg.tar.zst

cd /

pacman -U --noconfirm $F
