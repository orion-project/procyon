#!/usr/bin/env python3

"""Download and prepare Hunspell dictionaries."""

from __future__ import annotations

import argparse
import hashlib
import re
import sys
import tarfile
import tempfile
import urllib.error
import urllib.request
from pathlib import Path


LIBREOFFICE_REVISION = "f2ff99058268502bdcf4cad25c1ca2935ad8aa7d"
ARCHIVE_NAME = f"libreoffice-dictionaries-{LIBREOFFICE_REVISION}.tar.gz"
ARCHIVE_URL = (
    "https://github.com/LibreOffice/dictionaries/archive/"
    f"{LIBREOFFICE_REVISION}.tar.gz"
)
ARCHIVE_SHA512 = (
    "c3935525719a75077d7fa05378aabc0be2f8486c98fafb259043b831e16f8e93"
    "9c46dceef9077458149a515bc0bfb962fd7f31da66773eb8d87083e733fee76f"
)
ENCODING_PATTERN = re.compile(r"^SET\s+([^\s]+)", re.MULTILINE)


def parse_args() -> argparse.Namespace:
    script_dir = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(
        description="Download English and Russian Hunspell dictionaries."
    )
    parser.add_argument(
        "--archive",
        type=Path,
        default=script_dir / ARCHIVE_NAME,
        help="download cache location (default: %(default)s)",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=script_dir.parent / "bin" / "dicts",
        help="directory for the prepared dictionaries (default: %(default)s)",
    )
    parser.add_argument(
        "--redownload",
        action="store_true",
        help="download the archive again even when the cache already exists",
    )
    return parser.parse_args()


def download_archive(archive: Path, redownload: bool) -> None:
    if archive.is_file() and not redownload and archive_hash_matches(archive):
        print(f"Using cached archive: {archive}")
        return

    archive.parent.mkdir(parents=True, exist_ok=True)
    print(f"Downloading {ARCHIVE_URL}")
    request = urllib.request.Request(
        ARCHIVE_URL, headers={"User-Agent": "procyon-prepare-dict/1.0"}
    )
    with urllib.request.urlopen(request) as response:
        with tempfile.NamedTemporaryFile(
            mode="wb", dir=archive.parent, delete=False
        ) as temporary_file:
            temporary_path = Path(temporary_file.name)
            try:
                while True:
                    chunk = response.read(1024 * 1024)
                    if not chunk:
                        break
                    temporary_file.write(chunk)
                temporary_file.flush()
                temporary_path.replace(archive)
            except BaseException:
                if temporary_path.exists():
                    temporary_path.unlink()
                raise

    if not archive_hash_matches(archive):
        archive.unlink()
        raise RuntimeError("Downloaded archive does not match the expected SHA-512 hash")


def archive_hash_matches(archive: Path) -> bool:
    if not archive.is_file():
        return False

    checksum = hashlib.sha512()
    with archive.open("rb") as file:
        while True:
            chunk = file.read(1024 * 1024)
            if not chunk:
                break
            checksum.update(chunk)
    return checksum.hexdigest() == ARCHIVE_SHA512


def find_archive_root(archive: tarfile.TarFile) -> str:
    suffix = "/en/en_US.aff"
    roots = [name[:-len(suffix)] for name in archive.getnames() if name.endswith(suffix)]
    if len(roots) != 1:
        raise RuntimeError("Unable to find the English dictionary in the archive")
    return roots[0]


def read_member(archive: tarfile.TarFile, root: str, relative_path: str) -> bytes:
    member = f"{root}/{relative_path}"
    try:
        extracted_file = archive.extractfile(member)
    except KeyError as error:
        raise RuntimeError(f"Dictionary file is missing from the archive: {member}") from error
    if extracted_file is None:
        raise RuntimeError(f"Dictionary file is not a regular file: {member}")
    return extracted_file.read()


def prepare_dictionaries(archive_path: Path, output_dir: Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)

    with tarfile.open(archive_path, "r:gz") as archive:
        root = find_archive_root(archive)
        for extension in ("aff", "dic"):
            (output_dir / f"en_US.{extension}").write_bytes(
                read_member(archive, root, f"en/en_US.{extension}")
            )

        russian_affix_bytes = read_member(archive, root, "ru_RU/ru_RU.aff")
        encoding = dictionary_encoding(russian_affix_bytes)
        russian_affix = russian_affix_bytes.decode(encoding)
        if encoding.lower().replace("_", "-") != "utf-8":
            russian_affix = ENCODING_PATTERN.sub("SET UTF-8", russian_affix, count=1)
        (output_dir / "ru_RU.aff").write_text(russian_affix, encoding="utf-8")

        russian_dictionary = read_member(archive, root, "ru_RU/ru_RU.dic").decode(encoding)
        (output_dir / "ru_RU.dic").write_text(russian_dictionary, encoding="utf-8")


def dictionary_encoding(affix_data: bytes) -> str:
    affix_header = affix_data.decode("ascii", errors="replace")
    match = ENCODING_PATTERN.search(affix_header)
    if not match:
        raise RuntimeError("The Russian affix file does not declare an encoding with SET")

    encoding = match.group(1).lower()
    if encoding.startswith("microsoft-cp"):
        encoding = encoding.replace("microsoft-", "")
    try:
        "".encode(encoding)
    except LookupError as error:
        raise RuntimeError(f"Unsupported dictionary encoding: {match.group(1)}") from error
    return encoding


def main() -> int:
    args = parse_args()
    try:
        download_archive(args.archive, args.redownload)
        if not archive_hash_matches(args.archive):
            raise RuntimeError("Archive does not match the expected SHA-512 hash")
        prepare_dictionaries(args.archive, args.output_dir)
    except (OSError, RuntimeError, UnicodeError, urllib.error.URLError, tarfile.TarError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1

    print(f"Prepared dictionaries in {args.output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
