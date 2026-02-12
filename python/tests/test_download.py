"""Unit tests for the download module."""

from __future__ import annotations

import hashlib
import io
import sys
import tarfile
import urllib.error
import zipfile
from pathlib import Path
from unittest import mock

import pytest

from qtmcp.download import (
    AVAILABLE_VERSIONS,
    ChecksumError,
    DownloadError,
    UnsupportedPlatformError,
    VersionNotFoundError,
    _default_release_tag,
    build_archive_url,
    build_checksums_url,
    detect_platform,
    download_and_extract,
    extract_archive,
    get_archive_filename,
    get_launcher_filename,
    get_probe_filename,
    normalize_version,
    parse_checksums,
    verify_checksum,
)


class TestPlatformDetection:
    """Tests for platform detection logic."""

    def test_linux_platform_detection(self) -> None:
        """Linux platforms should return 'linux'."""
        with mock.patch("qtmcp.download.sys.platform", "linux"):
            assert detect_platform() == "linux"

    def test_linux2_platform_detection(self) -> None:
        """linux2 (older Python) should also work."""
        with mock.patch("qtmcp.download.sys.platform", "linux2"):
            assert detect_platform() == "linux"

    def test_win32_platform_detection(self) -> None:
        """Windows platforms should return 'windows'."""
        with mock.patch("qtmcp.download.sys.platform", "win32"):
            assert detect_platform() == "windows"

    def test_unsupported_platform_raises(self) -> None:
        """Unsupported platforms should raise UnsupportedPlatformError."""
        with mock.patch("qtmcp.download.sys.platform", "darwin"):
            with pytest.raises(UnsupportedPlatformError) as exc_info:
                detect_platform()
            assert "darwin" in str(exc_info.value)
            assert "Supported platforms" in str(exc_info.value)


class TestDefaultReleaseTag:
    """Tests for default release tag derivation."""

    def test_clean_version(self) -> None:
        """Clean version produces v-prefixed tag."""
        with mock.patch("qtmcp.download._pkg_version", return_value="0.2.0"):
            assert _default_release_tag() == "v0.2.0"

    def test_dev_suffix_stripped(self) -> None:
        """Dev suffix is stripped."""
        with mock.patch("qtmcp.download._pkg_version", return_value="0.3.0.dev5+gabcdef"):
            assert _default_release_tag() == "v0.3.0"

    def test_post_suffix_stripped(self) -> None:
        """Post suffix is stripped."""
        with mock.patch("qtmcp.download._pkg_version", return_value="0.2.0.post1"):
            assert _default_release_tag() == "v0.2.0"


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


class TestFilenames:
    """Tests for simplified filename generation."""

    def test_probe_filename_windows(self) -> None:
        """Windows probe filename."""
        assert get_probe_filename("windows") == "qtmcp-probe.dll"

    def test_probe_filename_linux(self) -> None:
        """Linux probe filename."""
        assert get_probe_filename("linux") == "qtmcp-probe.so"

    def test_probe_filename_auto_detect(self) -> None:
        """Probe filename auto-detects platform."""
        with mock.patch("qtmcp.download.sys.platform", "win32"):
            assert get_probe_filename() == "qtmcp-probe.dll"
        with mock.patch("qtmcp.download.sys.platform", "linux"):
            assert get_probe_filename() == "qtmcp-probe.so"

    def test_launcher_filename_windows(self) -> None:
        """Windows launcher filename."""
        assert get_launcher_filename("windows") == "qtmcp-launcher.exe"

    def test_launcher_filename_linux(self) -> None:
        """Linux launcher filename."""
        assert get_launcher_filename("linux") == "qtmcp-launcher"

    def test_launcher_filename_auto_detect(self) -> None:
        """Launcher filename auto-detects platform."""
        with mock.patch("qtmcp.download.sys.platform", "win32"):
            assert get_launcher_filename() == "qtmcp-launcher.exe"
        with mock.patch("qtmcp.download.sys.platform", "linux"):
            assert get_launcher_filename() == "qtmcp-launcher"


class TestArchiveFilename:
    """Tests for archive filename generation."""

    def test_windows_zip(self) -> None:
        """Windows archives should be .zip."""
        assert get_archive_filename("6.8", "windows") == "qtmcp-qt6.8-windows.zip"

    def test_linux_tar_gz(self) -> None:
        """Linux archives should be .tar.gz."""
        assert get_archive_filename("6.8", "linux") == "qtmcp-qt6.8-linux.tar.gz"

    def test_patched_version(self) -> None:
        """Patched versions should be preserved in filename."""
        assert get_archive_filename("5.15-patched", "linux") == "qtmcp-qt5.15-patched-linux.tar.gz"

    def test_version_normalization(self) -> None:
        """Full version strings should be normalized."""
        assert get_archive_filename("6.8.0", "windows") == "qtmcp-qt6.8-windows.zip"

    def test_auto_detect_platform(self) -> None:
        """Platform should be auto-detected when not specified."""
        with mock.patch("qtmcp.download.sys.platform", "win32"):
            assert get_archive_filename("6.8") == "qtmcp-qt6.8-windows.zip"


class TestArchiveUrlBuilding:
    """Tests for archive URL construction."""

    def test_build_archive_url_windows(self) -> None:
        """Build correct URL for Windows archive."""
        url = build_archive_url("6.8", release_tag="v0.3.0", platform_name="windows")
        assert url == (
            "https://github.com/ssss2art/QtMcp/releases/download/"
            "v0.3.0/qtmcp-qt6.8-windows.zip"
        )

    def test_build_archive_url_linux(self) -> None:
        """Build correct URL for Linux archive."""
        url = build_archive_url("6.8", release_tag="v0.3.0", platform_name="linux")
        assert url == (
            "https://github.com/ssss2art/QtMcp/releases/download/"
            "v0.3.0/qtmcp-qt6.8-linux.tar.gz"
        )

    def test_build_archive_url_patched(self) -> None:
        """Build correct URL for patched Qt version."""
        url = build_archive_url("5.15-patched", release_tag="v0.3.0", platform_name="linux")
        assert url == (
            "https://github.com/ssss2art/QtMcp/releases/download/"
            "v0.3.0/qtmcp-qt5.15-patched-linux.tar.gz"
        )

    def test_build_archive_url_invalid_version(self) -> None:
        """Invalid Qt version should raise VersionNotFoundError."""
        with pytest.raises(VersionNotFoundError) as exc_info:
            build_archive_url("6.0", release_tag="v0.3.0")
        assert "6.0" in str(exc_info.value)
        assert "Available versions" in str(exc_info.value)

    def test_build_archive_url_latest_uses_package_version(self) -> None:
        """'latest' release tag should resolve from package version."""
        with mock.patch("qtmcp.download._pkg_version", return_value="0.3.0"):
            url = build_archive_url("6.8", release_tag="latest", platform_name="linux")
            assert "/v0.3.0/" in url

    def test_build_archive_url_latest_strips_dev_suffix(self) -> None:
        """'latest' should strip dev suffixes from package version."""
        with mock.patch("qtmcp.download._pkg_version", return_value="0.3.0.dev5+gabcdef"):
            url = build_archive_url("6.8", release_tag="latest", platform_name="linux")
            assert "/v0.3.0/" in url

    def test_build_checksums_url(self) -> None:
        """Build correct URL for SHA256SUMS file."""
        url = build_checksums_url("v1.2.3")
        assert url == "https://github.com/ssss2art/QtMcp/releases/download/v1.2.3/SHA256SUMS"

    def test_build_checksums_url_latest_uses_package_version(self) -> None:
        """'latest' should resolve from package version for checksums."""
        with mock.patch("qtmcp.download._pkg_version", return_value="0.2.0"):
            url = build_checksums_url("latest")
            assert "/v0.2.0/" in url


class TestChecksumParsing:
    """Tests for SHA256SUMS file parsing."""

    def test_parse_standard_format(self) -> None:
        """Parse standard sha256sum output format."""
        content = """\
abc123def456  qtmcp-qt6.8-linux.tar.gz
789xyz012abc  qtmcp-qt6.8-windows.zip
"""
        checksums = parse_checksums(content)
        assert checksums["qtmcp-qt6.8-linux.tar.gz"] == "abc123def456"
        assert checksums["qtmcp-qt6.8-windows.zip"] == "789xyz012abc"

    def test_parse_binary_mode_format(self) -> None:
        """Parse sha256sum binary mode format (asterisk prefix)."""
        content = "abc123def456 *qtmcp-qt6.8-linux.tar.gz\n"
        checksums = parse_checksums(content)
        assert checksums["qtmcp-qt6.8-linux.tar.gz"] == "abc123def456"

    def test_parse_empty_lines_ignored(self) -> None:
        """Empty lines should be ignored."""
        content = "\nabc123def456  file.zip\n\n"
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


class TestExtractArchive:
    """Tests for archive extraction."""

    def test_extract_zip(self, tmp_path: Path) -> None:
        """Extract a zip archive with probe + launcher."""
        archive_path = tmp_path / "qtmcp-qt6.8-windows.zip"
        output_dir = tmp_path / "output"

        with zipfile.ZipFile(archive_path, "w") as zf:
            zf.writestr("qtmcp-probe.dll", b"probe binary")
            zf.writestr("qtmcp-launcher.exe", b"launcher binary")

        extracted = extract_archive(archive_path, output_dir)

        assert len(extracted) == 2
        assert (output_dir / "qtmcp-probe.dll").exists()
        assert (output_dir / "qtmcp-launcher.exe").exists()
        assert (output_dir / "qtmcp-probe.dll").read_bytes() == b"probe binary"
        assert (output_dir / "qtmcp-launcher.exe").read_bytes() == b"launcher binary"

    def test_extract_tar_gz(self, tmp_path: Path) -> None:
        """Extract a tar.gz archive with probe + launcher."""
        archive_path = tmp_path / "qtmcp-qt6.8-linux.tar.gz"
        output_dir = tmp_path / "output"

        with tarfile.open(archive_path, "w:gz") as tf:
            probe_data = b"probe binary"
            info = tarfile.TarInfo(name="qtmcp-probe.so")
            info.size = len(probe_data)
            tf.addfile(info, io.BytesIO(probe_data))

            launcher_data = b"launcher binary"
            info = tarfile.TarInfo(name="qtmcp-launcher")
            info.size = len(launcher_data)
            tf.addfile(info, io.BytesIO(launcher_data))

        extracted = extract_archive(archive_path, output_dir)

        assert len(extracted) == 2
        assert (output_dir / "qtmcp-probe.so").exists()
        assert (output_dir / "qtmcp-launcher").exists()

    def test_extract_zip_rejects_path_traversal(self, tmp_path: Path) -> None:
        """Zip archives with path traversal should be rejected."""
        archive_path = tmp_path / "evil.zip"
        output_dir = tmp_path / "output"

        with zipfile.ZipFile(archive_path, "w") as zf:
            zf.writestr("../../../etc/passwd", b"evil content")

        with pytest.raises(DownloadError, match="unsafe path"):
            extract_archive(archive_path, output_dir)

    def test_extract_tar_rejects_path_traversal(self, tmp_path: Path) -> None:
        """Tar archives with path traversal should be rejected."""
        archive_path = tmp_path / "evil.tar.gz"
        output_dir = tmp_path / "output"

        with tarfile.open(archive_path, "w:gz") as tf:
            data = b"evil content"
            info = tarfile.TarInfo(name="../../../etc/passwd")
            info.size = len(data)
            tf.addfile(info, io.BytesIO(data))

        with pytest.raises(DownloadError, match="unsafe path"):
            extract_archive(archive_path, output_dir)

    def test_extract_tar_rejects_symlinks(self, tmp_path: Path) -> None:
        """Tar archives with symlinks should be rejected."""
        archive_path = tmp_path / "evil.tar.gz"
        output_dir = tmp_path / "output"

        with tarfile.open(archive_path, "w:gz") as tf:
            info = tarfile.TarInfo(name="link")
            info.type = tarfile.SYMTYPE
            info.linkname = "/etc/passwd"
            tf.addfile(info)

        with pytest.raises(DownloadError, match="unsafe path"):
            extract_archive(archive_path, output_dir)

    def test_extract_unsupported_format(self, tmp_path: Path) -> None:
        """Unsupported archive formats should raise DownloadError."""
        archive_path = tmp_path / "file.rar"
        archive_path.write_bytes(b"not a real archive")
        output_dir = tmp_path / "output"

        with pytest.raises(DownloadError, match="Unsupported archive format"):
            extract_archive(archive_path, output_dir)

    def test_extract_bad_zip(self, tmp_path: Path) -> None:
        """Corrupt zip should raise DownloadError."""
        archive_path = tmp_path / "bad.zip"
        archive_path.write_bytes(b"not a zip file")
        output_dir = tmp_path / "output"

        with pytest.raises(DownloadError, match="Failed to extract"):
            extract_archive(archive_path, output_dir)


class TestDownloadAndExtract:
    """Tests for the main download_and_extract function."""

    def _make_zip(self, probe_data: bytes, launcher_data: bytes) -> bytes:
        """Create an in-memory zip archive."""
        buf = io.BytesIO()
        with zipfile.ZipFile(buf, "w") as zf:
            zf.writestr("qtmcp-probe.dll", probe_data)
            zf.writestr("qtmcp-launcher.exe", launcher_data)
        return buf.getvalue()

    def test_download_success_without_checksum(self, tmp_path: Path) -> None:
        """Download succeeds without checksum verification."""
        archive_data = self._make_zip(b"probe", b"launcher")

        def mock_urlopen(url: str, timeout: int | None = None) -> io.BytesIO:
            return io.BytesIO(archive_data)

        with mock.patch("qtmcp.download.sys.platform", "win32"):
            with mock.patch("qtmcp.download.urllib.request.urlopen", mock_urlopen):
                probe, launcher = download_and_extract(
                    "6.8",
                    output_dir=tmp_path,
                    verify=False,
                    release_tag="v0.3.0",
                )

        assert probe.exists()
        assert launcher.exists()
        assert probe.name == "qtmcp-probe.dll"
        assert launcher.name == "qtmcp-launcher.exe"
        assert probe.read_bytes() == b"probe"
        assert launcher.read_bytes() == b"launcher"

    def test_download_with_checksum_verification(self, tmp_path: Path) -> None:
        """Download verifies checksum when enabled."""
        archive_data = self._make_zip(b"probe", b"launcher")
        expected_hash = hashlib.sha256(archive_data).hexdigest()
        checksums_content = f"{expected_hash}  qtmcp-qt6.8-windows.zip\n"

        call_count = {"count": 0}

        def mock_urlopen(url: str, timeout: int | None = None) -> io.BytesIO:
            call_count["count"] += 1
            if "SHA256SUMS" in url:
                return io.BytesIO(checksums_content.encode())
            return io.BytesIO(archive_data)

        with mock.patch("qtmcp.download.sys.platform", "win32"):
            with mock.patch("qtmcp.download.urllib.request.urlopen", mock_urlopen):
                probe, launcher = download_and_extract(
                    "6.8",
                    output_dir=tmp_path,
                    verify=True,
                    release_tag="v0.3.0",
                )

        assert probe.exists()
        assert launcher.exists()
        assert call_count["count"] == 2  # SHA256SUMS + archive

    def test_download_checksum_mismatch_raises(self, tmp_path: Path) -> None:
        """Checksum mismatch should raise ChecksumError."""
        archive_data = self._make_zip(b"probe", b"launcher")
        wrong_hash = "0" * 64
        checksums_content = f"{wrong_hash}  qtmcp-qt6.8-windows.zip\n"

        def mock_urlopen(url: str, timeout: int | None = None) -> io.BytesIO:
            if "SHA256SUMS" in url:
                return io.BytesIO(checksums_content.encode())
            return io.BytesIO(archive_data)

        with mock.patch("qtmcp.download.sys.platform", "win32"):
            with mock.patch("qtmcp.download.urllib.request.urlopen", mock_urlopen):
                with pytest.raises(ChecksumError) as exc_info:
                    download_and_extract(
                        "6.8",
                        output_dir=tmp_path,
                        verify=True,
                        release_tag="v0.3.0",
                    )

        assert "verification failed" in str(exc_info.value)
        # Archive should be cleaned up
        assert not (tmp_path / "qtmcp-qt6.8-windows.zip").exists()

    def test_archive_cleaned_up_after_extraction(self, tmp_path: Path) -> None:
        """Archive file should be deleted after successful extraction."""
        archive_data = self._make_zip(b"probe", b"launcher")

        def mock_urlopen(url: str, timeout: int | None = None) -> io.BytesIO:
            return io.BytesIO(archive_data)

        with mock.patch("qtmcp.download.sys.platform", "win32"):
            with mock.patch("qtmcp.download.urllib.request.urlopen", mock_urlopen):
                download_and_extract(
                    "6.8",
                    output_dir=tmp_path,
                    verify=False,
                    release_tag="v0.3.0",
                )

        # Archive should be cleaned up
        assert not (tmp_path / "qtmcp-qt6.8-windows.zip").exists()
        # But extracted files should remain
        assert (tmp_path / "qtmcp-probe.dll").exists()
        assert (tmp_path / "qtmcp-launcher.exe").exists()


class TestErrorHandling:
    """Tests for error handling."""

    def test_404_error_handling(self, tmp_path: Path) -> None:
        """HTTP 404 should raise DownloadError."""
        def mock_urlopen(url: str, timeout: int | None = None) -> None:
            raise urllib.error.HTTPError(url, 404, "Not Found", {}, None)

        with mock.patch("qtmcp.download.sys.platform", "win32"):
            with mock.patch("qtmcp.download.urllib.request.urlopen", mock_urlopen):
                with pytest.raises(DownloadError) as exc_info:
                    download_and_extract(
                        "6.8",
                        output_dir=tmp_path,
                        verify=True,
                        release_tag="v0.3.0",
                    )

        assert "not found" in str(exc_info.value).lower()

    def test_network_error_handling(self, tmp_path: Path) -> None:
        """Network errors should raise DownloadError."""
        def mock_urlopen(url: str, timeout: int | None = None) -> None:
            raise urllib.error.URLError("Connection refused")

        with mock.patch("qtmcp.download.sys.platform", "win32"):
            with mock.patch("qtmcp.download.urllib.request.urlopen", mock_urlopen):
                with pytest.raises(DownloadError) as exc_info:
                    download_and_extract(
                        "6.8",
                        output_dir=tmp_path,
                        verify=True,
                        release_tag="v0.3.0",
                    )

        assert "network error" in str(exc_info.value).lower()


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
