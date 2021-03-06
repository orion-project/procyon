# Dependencies

All the commands below suppose the current directory is `deps`.

## hunspell

[hunspell](http://hunspell.github.io/) used for spell checking. See its official [readme](https://github.com/hunspell/hunspell) for full build instructions.

For Windows, the preferable way is using [MSYS2](http://www.msys2.org/). In this case, build commands are the same as for other platforms. But note that it takes about 2GB with all dev tools installed.

Install build tools if they are not presented:

```bash
# Linux
sudo apt install autoconf automake autopoint libtool

# macOS
brew install autoconf automake libtool gettext

# Windows/MSYS2
pacman -S base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-libtool unzip
```

Download, unpack, and build hunspell library:

```bash
curl https://codeload.github.com/hunspell/hunspell/zip/v1.7.0 > hunspell-1.7.0.zip
unzip -q hunspell-1.7.0.zip
cd hunspell-1.7.0
autoreconf -vfi
./configure
make
```

On macOS, we have to set a lower target os version than it is to avoid many linker warnings when linking lib to the application (the solution is from [StackOverflow](https://stackoverflow.com/questions/43216273/object-file-was-built-for-newer-osx-version-than-being-linked)):

> object file ... was built for newer OSX version (10.12) than being linked (10.10)

```bash
make CXXFLAGS="-g -O2 -mmacosx-version-min=10.10"
```

**NB/TODO:** On Windows, it'll be better to build hunspell using the same version of MinGW is used for building the application itself. There could be problems with different versions of std. It's is unable to statically link `hunspell*.a` to the main executable because of name resolution errors. `hunspell*.dll` is dynamically linked; however, it depends on `libgcc_s_seh-1.dll` and `libstdc++-6.dll` which are different in Qt's MinGW and MSYS's one.

**Download dictionaries**

In the example below, the package is the full set of dictionaries for [LibreOffice](https://github.com/LibreOffice/dictionaries) (74M), and we have to extract only needed `.dic` and `.aff` files.

```bash
# Don't forget to leave hunspell directory
cd ..

curl https://codeload.github.com/LibreOffice/dictionaries/zip/libreoffice-6.3.0.4 > libreoffice-6.3.0.4.zip
unzip -j libreoffice-6.3.0.4.zip dictionaries-libreoffice-6.3.0.4/en/en_US.* -d ../bin/dicts
unzip -j libreoffice-6.3.0.4.zip dictionaries-libreoffice-6.3.0.4/ru_RU/ru_RU.* -d ../bin/dicts
```

## hoedown

[hoedown](https://github.com/hoedown/hoedown) used to render [markdown](https://en.wikipedia.org/wiki/Markdown) memos to display HTML. It included into the project as source code files so not special build operations needed, you only have to clone the repo.

```bash
git clone https://github.com/hoedown/hoedown
cd hoedown
git checkout 3.0.7
cd ..
```
