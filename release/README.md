# Release

This directory contains some stuff used during preparation of release version of the application and making redistributable packages.

Commands for running scripts suppose that current dir is the project root - a directory containing `procyon.pro` file.

## Version format

```
MAJOR.MINOR.PATCH[-CODENAME]
```

See: [Semantic Versioning](https://semver.org)

## Prepare new release

### Update version info

Decide version numbers for the new release. Current version is stored in `./release/version.txt` file. Increase at least one of `MAJOR`, `MINOR`, or `PATCH` numbers when creating a new release. Update version info that is built into the application and make a new release tag:

```bash
 ./release/make_version.py 0.1.0
git commit -am 'Update version info to 0.1.0'
git tag -a v0.1.0 -m 'Short version description'
git push origin v0.1.0
```

### Build package

```bash
./release/build_release.py
./release/make_package.py
```

Target package is in `out` subdirectory, it's named
- on Windows `procyon-{version}.zip`
- on MacOS `procyon-{version}.dmg`
- on Linux: `procyon-{version}.AppImage`

To build packages on different platform, pull the latest version information and tag:

```bash
git pull --prune --tags
git pull
```
