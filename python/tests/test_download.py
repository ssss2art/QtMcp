"""Unit tests for the download module."""

from __future__ import annotations

import hashlib
import io
import urllib.error
from pathlib import Path
from typing import TYPE_CHECKING
from unittest import mock

import pytest

from qtmcp.download import (
    AVAILABLE_VERSIONS,
    ChecksumError,
    DownloadError,
    UnsupportedPlatformError,
    VersionNotFoundError,
    build_checksums_url,
    build_probe_url,
    detect_platform,
    download_probe,
    get_probe_filename,
    normalize_version,
    parse_checksums,
    verify_checksum,
)


class TestPlatformDetection:
    """Tests for platform detection logic."""

    def test_linux_platform_detection(self) -> None:
        """Linux platforms should map to linux-gcc suffix."""
        with mock.patch("qtmcp.download.sys.platform", "linux"):
            suffix, ext = detect_platform()
            assert suffix == "linux-gcc"
            assert ext == "so"

    def test_linux2_platform_detection(self) -> None:
        """linux2 (older Python) should also work."""
        with mock.patch("qtmcp.download.sys.platform", "linux2"):
            suffix, ext = detect_platform()
            assert suffix == "linux-gcc"
            assert ext == "so"

    def test_win32_platform_detection(self) -> None:
        """Windows platforms should map to windows-msvc suffix."""
        with mock.patch("qtmcp.download.sys.platform", "win32"):
            suffix, ext = detect_platform()
            assert suffix == "windows-msvc"
            assert ext == "dll"

    def test_unsupported_platform_raises(self) -> None:
        """Unsupported platforms should raise UnsupportedPlatformError."""
        with mock.patch("qtmcp.download.sys.platform", "darwin"):
            with pytest.raises(UnsupportedPlatformError) as exc_info:
                detect_platform()
            assert "darwin" in str(exc_info.value)
            assert "Supported platforms" in str(exc_info.value)


class TestVersionNormalization:
    """Tests for Qt version normalization."""

    def test_short_version(self) -> None:
        """Short versions pass through unchanged."""
        assert normalize_version("6.8") == "6.8"

    def test_long_version_trimmed(self) -> None:
        """Patch versions are trimmed to major.minor."""
        assert normalize_version("6.8.0") == "6.8"
        assert normalize_version("5.15.2") == "5.15"

    def test_patched_suffix_preserved(self) -> None:
        """Patched suffix is preserved after normalization."""
        assert normalize_version("5.15-patched") == "5.15-patched"
        assert normalize_version("5.15.1-patched") == "5.15-patched"


class TestUrlBuilding:
    """Tests for URL building logic."""

    def test_build_probe_url_linux(self) -> None:
        """Build correct URL for Linux platform."""
        url = build_probe_url(
            "6.8", release_tag="v1.0.0", platform_suffix="linux-gcc", extension="so"
        )
        assert url == "https://github.com/ssss2art/QtMcp/releases/download/v1.0.0/qtmcp-probe-qt6.8-linux-gcc.so"

    def test_build_probe_url_windows(self) -> None:
        """Build correct URL for Windows platform."""
        url = build_probe_url(
            "6.8", release_tag="v1.0.0", platform_suffix="windows-msvc", extension="dll"
        )
        assert url == "https://github.com/ssss2art/QtMcp/releases/download/v1.0.0/qtmcp-probe-qt6.8-windows-msvc.dll"

    def test_build_probe_url_patched(self) -> None:
        """Build correct URL for patched Qt version."""
        url = build_probe_url(
            "5.15-patched", release_tag="v1.0.0", platform_suffix="linux-gcc", extension="so"
        )
        assert url == "https://github.com/ssss2art/QtMcp/releases/download/v1.0.0/qtmcp-probe-qt5.15-patched-linux-gcc.so"

    def test_build_probe_url_invalid_version(self) -> None:
        """Invalid Qt version should raise VersionNotFoundError."""
        with pytest.raises(VersionNotFoundError) as exc_info:
            build_probe_url("6.0", release_tag="v1.0.0")
        assert "6.0" in str(exc_info.value)
        assert "Available versions" in str(exc_info.value)

    def test_build_checksums_url(self) -> None:
        """Build correct URL for SHA256SUMS file."""
        url = build_checksums_url("v1.2.3")
        assert url == "https://github.com/ssss2art/QtMcp/releases/download/v1.2.3/SHA256SUMS"


class TestChecksumParsing:
    """Tests for SHA256SUMS file parsing."""

    def test_parse_standard_format(self) -> None:
        """Parse standard sha256sum output format."""
        content = """\
abc123def456  qtmcp-probe-qt6.8-linux-gcc.so
789xyz012abc  qtmcp-probe-qt6.8-windows-msvc.dll
"""
        checksums = parse_checksums(content)
        assert checksums["qtmcp-probe-qt6.8-linux-gcc.so"] == "abc123def456"
        assert checksums["qtmcp-probe-qt6.8-windows-msvc.dll"] == "789xyz012abc"

    def test_parse_binary_mode_format(self) -> None:
        """Parse sha256sum binary mode format (asterisk prefix)."""
        content = "abc123def456 *qtmcp-probe-qt6.8-linux-gcc.so\n"
        checksums = parse_checksums(content)
        assert checksums["qtmcp-probe-qt6.8-linux-gcc.so"] == "abc123def456"

    def test_parse_empty_lines_ignored(self) -> None:
        """Empty lines should be ignored."""
        content = "\nabc123def456  file.so\n\n"
        checksums = parse_checksums(content)
        assert len(checksums) == 1


class TestChecksumVerification:
    """Tests for checksum verification."""

    def test_checksum_match(self, tmp_path: Path) -> None:
        """Checksum should match for correct file."""
        content = b"test file content"
        expected_hash = hashlib.sha256(content).hexdigest()

        test_file = tmp_path / "test.bin"
        test_file.write_bytes(content)

        assert verify_checksum(test_file, expected_hash) is True

    def test_checksum_mismatch(self, tmp_path: Path) -> None:
        """Checksum should not match for incorrect file."""
        test_file = tmp_path / "test.bin"
        test_file.write_bytes(b"test content")

        wrong_hash = "0" * 64
        assert verify_checksum(test_file, wrong_hash) is False

    def test_checksum_case_insensitive(self, tmp_path: Path) -> None:
        """Checksum comparison should be case-insensitive."""
        content = b"test"
        expected_hash = hashlib.sha256(content).hexdigest().upper()

        test_file = tmp_path / "test.bin"
        test_file.write_bytes(content)

        assert verify_checksum(test_file, expected_hash) is True


class TestDownloadProbe:
    """Tests for the main download_probe function."""

    def test_download_success_without_checksum(self, tmp_path: Path) -> None:
        """Download succeeds without checksum verification."""
        fake_content = b"fake probe binary"

        def mock_urlopen(url: str, timeout: int | None = None) -> io.BytesIO:
            return io.BytesIO(fake_content)

        with mock.patch("qtmcp.download.sys.platform", "win32"):
            with mock.patch("qtmcp.download.urllib.request.urlopen", mock_urlopen):
                path = download_probe(
                    "6.8",
                    output_dir=tmp_path,
                    verify_checksum_flag=False,
                    release_tag="v1.0.0",
                )

        assert path.exists()
        assert path.read_bytes() == fake_content
        assert "qt6.8" in path.name
        assert "windows-msvc" in path.name
        assert path.suffix == ".dll"

    def test_download_with_checksum_verification(self, tmp_path: Path) -> None:
        """Download verifies checksum when enabled."""
        fake_content = b"fake probe binary"
        expected_hash = hashlib.sha256(fake_content).hexdigest()
        checksums_content = f"{expected_hash}  qtmcp-probe-qt6.8-windows-msvc.dll\n"

        call_count = {"count": 0}

        def mock_urlopen(url: str, timeout: int | None = None) -> io.BytesIO:
            call_count["count"] += 1
            if "SHA256SUMS" in url:
                return io.BytesIO(checksums_content.encode())
            return io.BytesIO(fake_content)

        with mock.patch("qtmcp.download.sys.platform", "win32"):
            with mock.patch("qtmcp.download.urllib.request.urlopen", mock_urlopen):
                path = download_probe(
                    "6.8",
                    output_dir=tmp_path,
                    verify_checksum_flag=True,
                    release_tag="v1.0.0",
                )

        assert path.exists()
        assert call_count["count"] == 2  # SHA256SUMS + probe binary

    def test_download_checksum_mismatch_raises(self, tmp_path: Path) -> None:
        """Checksum mismatch should raise ChecksumError."""
        fake_content = b"fake probe binary"
        wrong_hash = "0" * 64
        checksums_content = f"{wrong_hash}  qtmcp-probe-qt6.8-windows-msvc.dll\n"

        def mock_urlopen(url: str, timeout: int | None = None) -> io.BytesIO:
            if "SHA256SUMS" in url:
                return io.BytesIO(checksums_content.encode())
            return io.BytesIO(fake_content)

        with mock.patch("qtmcp.download.sys.platform", "win32"):
            with mock.patch("qtmcp.download.urllib.request.urlopen", mock_urlopen):
                with pytest.raises(ChecksumError) as exc_info:
                    download_probe(
                        "6.8",
                        output_dir=tmp_path,
                        verify_checksum_flag=True,
                        release_tag="v1.0.0",
                    )

        assert "verification failed" in str(exc_info.value)
        # Corrupted file should be deleted
        assert not (tmp_path / "qtmcp-probe-qt6.8-windows-msvc.dll").exists()


class TestErrorHandling:
    """Tests for error handling."""

    def test_404_error_handling(self, tmp_path: Path) -> None:
        """HTTP 404 should raise DownloadError."""
        def mock_urlopen(url: str, timeout: int | None = None) -> None:
            raise urllib.error.HTTPError(url, 404, "Not Found", {}, None)

        with mock.patch("qtmcp.download.sys.platform", "win32"):
            with mock.patch("qtmcp.download.urllib.request.urlopen", mock_urlopen):
                with pytest.raises(DownloadError) as exc_info:
                    download_probe(
                        "6.8",
                        output_dir=tmp_path,
                        verify_checksum_flag=True,
                        release_tag="v1.0.0",
                    )

        assert "not found" in str(exc_info.value).lower()

    def test_network_error_handling(self, tmp_path: Path) -> None:
        """Network errors should raise DownloadError."""
        def mock_urlopen(url: str, timeout: int | None = None) -> None:
            raise urllib.error.URLError("Connection refused")

        with mock.patch("qtmcp.download.sys.platform", "win32"):
            with mock.patch("qtmcp.download.urllib.request.urlopen", mock_urlopen):
                with pytest.raises(DownloadError) as exc_info:
                    download_probe(
                        "6.8",
                        output_dir=tmp_path,
                        verify_checksum_flag=True,
                        release_tag="v1.0.0",
                    )

        assert "network error" in str(exc_info.value).lower()


class TestGetProbeFilename:
    """Tests for get_probe_filename helper."""

    def test_get_filename_windows(self) -> None:
        """Get correct filename for Windows."""
        with mock.patch("qtmcp.download.sys.platform", "win32"):
            filename = get_probe_filename("6.8")
            assert filename == "qtmcp-probe-qt6.8-windows-msvc.dll"

    def test_get_filename_linux(self) -> None:
        """Get correct filename for Linux."""
        with mock.patch("qtmcp.download.sys.platform", "linux"):
            filename = get_probe_filename("5.15-patched")
            assert filename == "qtmcp-probe-qt5.15-patched-linux-gcc.so"


class TestAvailableVersions:
    """Tests for available versions constant."""

    def test_expected_versions_available(self) -> None:
        """All expected Qt versions should be available."""
        assert "5.15" in AVAILABLE_VERSIONS
        assert "5.15-patched" in AVAILABLE_VERSIONS
        assert "6.5" in AVAILABLE_VERSIONS
        assert "6.8" in AVAILABLE_VERSIONS
        assert "6.9" in AVAILABLE_VERSIONS

    def test_versions_is_frozen(self) -> None:
        """AVAILABLE_VERSIONS should be immutable."""
        assert isinstance(AVAILABLE_VERSIONS, frozenset)
