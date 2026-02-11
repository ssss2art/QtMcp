"""Download QtMCP probe binaries from GitHub Releases.

This module provides utilities to download the correct probe binary
for your Qt version and platform.
"""

from __future__ import annotations

import hashlib
import sys
import urllib.error
import urllib.request
from pathlib import Path
from typing import Literal

# GitHub repository for QtMCP releases
GITHUB_REPO = "ssss2art/QtMcp"
RELEASES_URL = f"https://github.com/{GITHUB_REPO}/releases/download"

# Available Qt versions (release workflow builds these)
AVAILABLE_VERSIONS = frozenset([
    "5.15",
    "5.15-patched",
    "6.5",
    "6.8",
    "6.9",
])

# Platform mapping (platform name only, no compiler)
PLATFORM_MAP: dict[str, tuple[str, str]] = {
    "linux": ("linux", "so"),
    "win32": ("windows", "dll"),
}

# Default compiler per platform (compiler + major version)
DEFAULT_COMPILERS: dict[str, str] = {
    "linux": "gcc13",
    "windows": "msvc17",
}


class DownloadError(Exception):
    """Raised when probe download fails."""


class ChecksumError(DownloadError):
    """Raised when checksum verification fails."""


class UnsupportedPlatformError(DownloadError):
    """Raised when the current platform is not supported."""


class VersionNotFoundError(DownloadError):
    """Raised when the requested Qt version is not available."""


def detect_platform() -> tuple[str, str]:
    """Detect the current platform and return (platform_name, file_extension).

    Returns:
        Tuple of (platform_name, file_extension) e.g. ("linux", "so")

    Raises:
        UnsupportedPlatformError: If the current platform is not supported.
    """
    platform = sys.platform
    if platform.startswith("linux"):
        platform = "linux"

    if platform not in PLATFORM_MAP:
        supported = ", ".join(PLATFORM_MAP.keys())
        raise UnsupportedPlatformError(
            f"Unsupported platform: {platform}. Supported platforms: {supported}"
        )

    return PLATFORM_MAP[platform]


def default_compiler(platform_name: str | None = None) -> str:
    """Get the default compiler for a platform.

    Args:
        platform_name: Platform name (e.g., "linux", "windows").
            Auto-detected if None.

    Returns:
        Default compiler string (e.g., "gcc13", "msvc17")

    Raises:
        UnsupportedPlatformError: If platform detection fails.
    """
    if platform_name is None:
        platform_name, _ = detect_platform()
    return DEFAULT_COMPILERS[platform_name]


def normalize_version(qt_version: str) -> str:
    """Normalize Qt version string.

    Args:
        qt_version: Version like "6.8", "6.8.0", "5.15-patched"

    Returns:
        Normalized version like "6.8" or "5.15-patched"
    """
    # Handle patched suffix
    patched = qt_version.endswith("-patched")
    if patched:
        qt_version = qt_version[:-8]  # Remove "-patched"

    # Split and normalize (e.g., "6.8.0" -> "6.8")
    parts = qt_version.split(".")
    if len(parts) >= 2:
        normalized = f"{parts[0]}.{parts[1]}"
    else:
        normalized = qt_version

    # Re-add patched suffix if present
    if patched:
        normalized = f"{normalized}-patched"

    return normalized


def build_probe_url(
    qt_version: str,
    release_tag: str = "latest",
    platform: str | None = None,
    extension: str | None = None,
    compiler: str | None = None,
) -> str:
    """Build the URL for a probe binary.

    Args:
        qt_version: Qt version (e.g., "6.8", "5.15-patched")
        release_tag: Release tag (e.g., "v0.1.0") or "latest"
        platform: Platform name (auto-detected if None)
        extension: File extension (auto-detected if None)
        compiler: Compiler suffix (e.g., "gcc13", "msvc17"). Auto-detected if None.

    Returns:
        URL to the probe binary

    Raises:
        UnsupportedPlatformError: If platform detection fails
        VersionNotFoundError: If the Qt version is not available
    """
    version = normalize_version(qt_version)

    if version not in AVAILABLE_VERSIONS:
        available = ", ".join(sorted(AVAILABLE_VERSIONS))
        raise VersionNotFoundError(
            f"Qt version '{version}' not available. Available versions: {available}"
        )

    if platform is None or extension is None:
        detected_platform, detected_ext = detect_platform()
        platform = platform or detected_platform
        extension = extension or detected_ext

    if compiler is None:
        compiler = default_compiler(platform)

    filename = f"qtmcp-probe-qt{version}-{platform}-{compiler}.{extension}"

    # Note: "latest" tag support would require GitHub API to resolve
    # For now, require explicit version tag
    if release_tag == "latest":
        release_tag = "v0.1.0"

    return f"{RELEASES_URL}/{release_tag}/{filename}"


def build_checksums_url(release_tag: str = "latest") -> str:
    """Build the URL for the SHA256SUMS file.

    Args:
        release_tag: Release tag (e.g., "v1.0.0") or "latest"

    Returns:
        URL to the SHA256SUMS file
    """
    if release_tag == "latest":
        release_tag = "v0.1.0"
    return f"{RELEASES_URL}/{release_tag}/SHA256SUMS"


def parse_checksums(content: str) -> dict[str, str]:
    """Parse SHA256SUMS file content.

    Args:
        content: Content of SHA256SUMS file

    Returns:
        Dict mapping filename to SHA256 hash
    """
    checksums = {}
    for line in content.strip().split("\n"):
        line = line.strip()
        if not line:
            continue
        # Format: "hash  filename" or "hash *filename" (binary mode)
        parts = line.split()
        if len(parts) >= 2:
            hash_value = parts[0]
            filename = parts[-1].lstrip("*")
            checksums[filename] = hash_value
    return checksums


def verify_checksum(filepath: Path, expected_hash: str) -> bool:
    """Verify file SHA256 checksum.

    Args:
        filepath: Path to the file
        expected_hash: Expected SHA256 hash (hex string)

    Returns:
        True if checksum matches
    """
    sha256 = hashlib.sha256()
    with open(filepath, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            sha256.update(chunk)
    return sha256.hexdigest().lower() == expected_hash.lower()


def download_file(url: str, output_path: Path) -> None:
    """Download a file from URL.

    Args:
        url: URL to download
        output_path: Local path to save file

    Raises:
        DownloadError: If download fails
    """
    try:
        # Create parent directories if needed
        output_path.parent.mkdir(parents=True, exist_ok=True)

        # Download with progress indication
        with urllib.request.urlopen(url, timeout=60) as response:
            with open(output_path, "wb") as f:
                while True:
                    chunk = response.read(8192)
                    if not chunk:
                        break
                    f.write(chunk)
    except urllib.error.HTTPError as e:
        if e.code == 404:
            raise DownloadError(f"File not found: {url}") from e
        raise DownloadError(f"HTTP error {e.code}: {url}") from e
    except urllib.error.URLError as e:
        raise DownloadError(f"Network error downloading {url}: {e.reason}") from e


def download_probe(
    qt_version: str,
    output_dir: Path | str | None = None,
    verify_checksum_flag: bool = True,
    release_tag: str = "latest",
    compiler: str | None = None,
) -> Path:
    """Download the QtMCP probe binary for the specified Qt version.

    Downloads the appropriate probe binary (DLL or SO) from GitHub Releases
    based on the Qt version and current platform. Optionally verifies the
    SHA256 checksum.

    Args:
        qt_version: Qt version (e.g., "6.8", "5.15", "5.15-patched")
        output_dir: Directory to save the probe (default: current directory)
        verify_checksum_flag: Whether to verify SHA256 checksum (default: True)
        release_tag: Release tag to download from (default: "latest")
        compiler: Compiler suffix (e.g., "gcc13", "msvc17"). Auto-detected if None.

    Returns:
        Path to the downloaded probe binary

    Raises:
        DownloadError: If download fails
        ChecksumError: If checksum verification fails
        UnsupportedPlatformError: If current platform is not supported
        VersionNotFoundError: If Qt version is not available

    Example:
        >>> probe_path = download_probe("6.8")
        >>> print(f"Downloaded probe to: {probe_path}")
    """
    # Normalize version
    version = normalize_version(qt_version)

    # Detect platform
    platform_name, extension = detect_platform()

    # Resolve compiler
    if compiler is None:
        compiler = default_compiler(platform_name)

    # Build URLs
    probe_url = build_probe_url(version, release_tag, platform_name, extension, compiler)

    # Determine output path
    if output_dir is None:
        output_dir = Path.cwd()
    else:
        output_dir = Path(output_dir)

    filename = f"qtmcp-probe-qt{version}-{platform_name}-{compiler}.{extension}"
    output_path = output_dir / filename

    # Download checksums first if verification enabled
    expected_hash: str | None = None
    if verify_checksum_flag:
        checksums_url = build_checksums_url(release_tag)
        try:
            with urllib.request.urlopen(checksums_url, timeout=30) as response:
                checksums_content = response.read().decode("utf-8")
            checksums = parse_checksums(checksums_content)
            expected_hash = checksums.get(filename)
            if expected_hash is None:
                raise DownloadError(
                    f"Checksum not found for {filename} in SHA256SUMS"
                )
        except urllib.error.HTTPError as e:
            if e.code == 404:
                raise DownloadError(
                    f"SHA256SUMS not found for release {release_tag}"
                ) from e
            raise DownloadError(
                f"Error downloading checksums: HTTP {e.code}"
            ) from e
        except urllib.error.URLError as e:
            raise DownloadError(
                f"Network error downloading checksums: {e.reason}"
            ) from e

    # Download probe binary
    download_file(probe_url, output_path)

    # Verify checksum if enabled
    if verify_checksum_flag and expected_hash:
        if not verify_checksum(output_path, expected_hash):
            # Remove corrupted file
            output_path.unlink(missing_ok=True)
            raise ChecksumError(
                f"Checksum verification failed for {filename}. "
                "File may be corrupted or tampered with."
            )

    return output_path


def get_probe_filename(qt_version: str, compiler: str | None = None) -> str:
    """Get the expected probe filename for a Qt version on current platform.

    Args:
        qt_version: Qt version (e.g., "6.8", "5.15-patched")
        compiler: Compiler suffix (e.g., "gcc13", "msvc17"). Auto-detected if None.

    Returns:
        Expected filename (e.g., "qtmcp-probe-qt6.8-windows-msvc17.dll")
    """
    version = normalize_version(qt_version)
    platform_name, extension = detect_platform()
    if compiler is None:
        compiler = default_compiler(platform_name)
    return f"qtmcp-probe-qt{version}-{platform_name}-{compiler}.{extension}"
