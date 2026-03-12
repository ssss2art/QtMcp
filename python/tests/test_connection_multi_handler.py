"""Unit tests for ProbeConnection multi-handler and call observer support."""

from __future__ import annotations

import asyncio

import pytest

pytestmark = pytest.mark.asyncio


class TestMultiNotificationHandlers:
    async def test_multiple_handlers_receive_notification(self, mock_probe):
        """All registered handlers receive the same notification."""
        probe, mock_ws = mock_probe
        received_a = []
        received_b = []

        probe.add_notification_handler(lambda m, p: received_a.append(m))
        probe.add_notification_handler(lambda m, p: received_b.append(m))

        await mock_ws.inject_notification({
            "jsonrpc": "2.0",
            "method": "qtmcp.signalEmitted",
            "params": {"objectId": "btn", "signal": "clicked"},
        })
        await asyncio.sleep(0.1)

        assert len(received_a) == 1
        assert len(received_b) == 1
        assert received_a[0] == "qtmcp.signalEmitted"

    async def test_remove_handler(self, mock_probe):
        """Removed handler stops receiving notifications."""
        probe, mock_ws = mock_probe
        received = []

        handler = lambda m, p: received.append(m)
        probe.add_notification_handler(handler)

        await mock_ws.inject_notification({
            "jsonrpc": "2.0",
            "method": "qtmcp.signalEmitted",
            "params": {},
        })
        await asyncio.sleep(0.1)
        assert len(received) == 1

        probe.remove_notification_handler(handler)

        await mock_ws.inject_notification({
            "jsonrpc": "2.0",
            "method": "qtmcp.signalEmitted",
            "params": {},
        })
        await asyncio.sleep(0.1)
        assert len(received) == 1  # no new

    async def test_remove_unregistered_handler_is_noop(self, mock_probe):
        """Removing a handler that was never added does not raise."""
        probe, mock_ws = mock_probe
        probe.remove_notification_handler(lambda m, p: None)  # no error

    async def test_crashing_handler_does_not_kill_others(self, mock_probe):
        """One handler raising does not prevent other handlers from running."""
        probe, mock_ws = mock_probe
        received = []

        def bad_handler(m, p):
            raise RuntimeError("boom")

        probe.add_notification_handler(bad_handler)
        probe.add_notification_handler(lambda m, p: received.append(m))

        await mock_ws.inject_notification({
            "jsonrpc": "2.0",
            "method": "qtmcp.signalEmitted",
            "params": {},
        })
        await asyncio.sleep(0.1)

        assert len(received) == 1  # second handler still ran

    async def test_on_notification_backward_compat(self, mock_probe):
        """Legacy on_notification() still works — clears list, sets one handler."""
        probe, mock_ws = mock_probe
        received = []

        with pytest.warns(DeprecationWarning):
            probe.on_notification(lambda m, p: received.append(m))

        await mock_ws.inject_notification({
            "jsonrpc": "2.0",
            "method": "qtmcp.signalEmitted",
            "params": {},
        })
        await asyncio.sleep(0.1)
        assert len(received) == 1

    async def test_on_notification_clears_add_handlers(self, mock_probe):
        """Calling on_notification() destroys handlers added via add_notification_handler."""
        probe, mock_ws = mock_probe
        received_add = []
        received_on = []

        probe.add_notification_handler(lambda m, p: received_add.append(m))
        with pytest.warns(DeprecationWarning):
            probe.on_notification(lambda m, p: received_on.append(m))

        await mock_ws.inject_notification({
            "jsonrpc": "2.0",
            "method": "qtmcp.signalEmitted",
            "params": {},
        })
        await asyncio.sleep(0.1)

        assert len(received_add) == 0  # cleared by on_notification
        assert len(received_on) == 1

    async def test_on_notification_emits_deprecation_warning(self, mock_probe):
        """on_notification() emits a DeprecationWarning."""
        probe, mock_ws = mock_probe
        with pytest.warns(DeprecationWarning, match="add_notification_handler"):
            probe.on_notification(lambda m, p: None)

    async def test_on_notification_none_clears_all(self, mock_probe):
        """on_notification(None) clears the handler list."""
        probe, mock_ws = mock_probe
        received = []

        probe.add_notification_handler(lambda m, p: received.append(m))
        with pytest.warns(DeprecationWarning):
            probe.on_notification(None)

        await mock_ws.inject_notification({
            "jsonrpc": "2.0",
            "method": "qtmcp.signalEmitted",
            "params": {},
        })
        await asyncio.sleep(0.1)
        assert len(received) == 0
