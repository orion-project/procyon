# Release

This directory contains some stuff used during preparation of release version of the application and making redistributable packages.

Commands for running scripts suppose that current dir is `release`, the directory containing this file. On Linux and MacOS you have to mark scripts as executables to be able to run them:

```bash
chmod +x make_*.py
```

## Version format

```
MAJOR.MINOR.PATCH[-CODENAME]
```

See: [Semantic Versioning](https://semver.org)

## Prepare new release

### Update version info

Decide version numbers for the new release. Current version is stored in `version.txt` file. Increase at least one of `MAJOR`, `MINOR`, or `PATCH` numbers when creating a new release. Update version info that is built into the application and make a new release tag:

```bash
 ./make_version.py 0.1.0
git commit -am 'Update version info to 0.1.0'
git tag -a v0.1.0 -m 'Short version description'
git push origin v0.1.0
```

### Build package

```bash
./make_release.py
./make_package.py
```

Target package is in `../out` subdirectory, it's named

- on Windows `procyon-{version}-win-{x32|x64}.zip`
- on Linux: `procyon-{version}-linux-{x32|x64}.AppImage`
- on MacOS `procyon-{version}.dmg`

### Notes

#### `qmake` version

Though the build scripts check if qmake is in PATH, you have to be sure it is proper qmake version. For example, there could be available command `/usr/bin/qmake` on Ubuntu. But in general, for development, I use different Qt version installed in the home directory, e.g., `~/Qt/5.10.0/`. In this case, scripts' check for qmake will pass, but it highly probable won't be able to build the app. If it is the case, add proper Qt version into PATH before running build scripts:

```bash
export PATH=/home/user/Qt/5.10.0/gcc_64/bin/:$PATH
./make_release.py
```