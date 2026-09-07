// Copyright (c) 2024 qtPilot Contributors
// SPDX-License-Identifier: MIT

#include "transport/websocket_server.h"

#include <QCoreApplication>
#include <QSignalSpy>
#include <QWebSocket>
#include <QtTest>

using namespace qtPilot;

/// Teardown of a server that still has a client attached.
///
/// This is R9 in docs/observability-testability-gaps.md. A live-client case
/// SEGFAULTed on Qt 6.8/6.9 (Linux and Windows both) while passing on 5.15.2,
/// 6.5.3, 6.10.0 and 6.11.1, and was removed rather than left flaky.
///
/// The mechanism is a re-entrancy hole in stop():
///
///     if (m_activeClient) {
///       ...
///       m_activeClient->close();        // may emit disconnected() SYNCHRONOUSLY
///       m_activeClient->deleteLater();  // m_activeClient may now be nullptr
///       m_activeClient = nullptr;
///     }
///
/// close() re-enters onClientDisconnected() through the queued/direct
/// disconnected() signal; that slot nulls m_activeClient, and the continuation
/// above then calls deleteLater() on a null pointer. Whether close() emits
/// synchronously depends on the socket's state and on the Qt version, which is
/// why only two Qt minors ever crashed.
///
/// These tests pin the teardown ORDER rather than the crash: state is detached
/// before anything that can re-enter, so the sequence is safe on every version
/// regardless of when Qt chooses to emit.
class TestWebSocketTeardown : public QObject {
  Q_OBJECT

 private:
  static constexpr quint16 kEphemeral = 0;

  /// Connect a real client and wait until the server has accepted it.
  static bool attachClient(WebSocketServer& server, QWebSocket& client) {
    QSignalSpy accepted(&server, &WebSocketServer::clientConnected);
    client.open(QUrl(QStringLiteral("ws://127.0.0.1:%1").arg(server.port())));
    return accepted.wait(5000);
  }

 private slots:
  void init() { qunsetenv("QTPILOT_BIND_ADDRESS"); }

  /// The original R9 reproducer: destroy a server while a client is attached.
  void destroyingAServerWithALiveClientIsSafe() {
    QWebSocket client;
    {
      WebSocketServer server(kEphemeral);
      QVERIFY(server.start());
      QVERIFY2(attachClient(server, client), "server never accepted the client");
      QVERIFY(server.hasActiveClient());
      // Destructor runs here, with the client still attached.
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
  }

  /// stop() called explicitly with a client attached -- the same path, reached
  /// without relying on destruction order.
  void stoppingWithALiveClientIsSafe() {
    WebSocketServer server(kEphemeral);
    QVERIFY(server.start());

    QWebSocket client;
    QVERIFY(attachClient(server, client));

    server.stop();
    QVERIFY(!server.hasActiveClient());
    QVERIFY(!server.isListening());
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
  }

  /// stop() must be idempotent: the destructor calls it after any explicit call.
  void stopIsIdempotentWithALiveClient() {
    WebSocketServer server(kEphemeral);
    QVERIFY(server.start());

    QWebSocket client;
    QVERIFY(attachClient(server, client));

    server.stop();
    server.stop();
    server.stop();
    QVERIFY(!server.hasActiveClient());
  }

  /// A client that vanishes without a close handshake. abort() drops the socket
  /// immediately, so the server's peer may already be dead when close() runs --
  /// the state most likely to make close() emit disconnected() synchronously.
  void stoppingAfterAnAbortedClientIsSafe() {
    WebSocketServer server(kEphemeral);
    QVERIFY(server.start());

    QWebSocket client;
    QVERIFY(attachClient(server, client));

    client.abort();
    server.stop();  // deliberately no event-loop turn in between
    QVERIFY(!server.hasActiveClient());
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
  }

  /// Re-entrant stop(): a clientDisconnected handler that stops the server.
  /// Nothing in the probe does this today, but it is the same shape as the
  /// synchronous-emit path and costs nothing to pin.
  void stopFromWithinTheDisconnectedHandlerIsSafe() {
    WebSocketServer server(kEphemeral);
    QVERIFY(server.start());

    connect(&server, &WebSocketServer::clientDisconnected, &server, [&server]() { server.stop(); });

    QWebSocket client;
    QVERIFY(attachClient(server, client));

    client.close();
    QTest::qWait(300);
    QVERIFY(!server.hasActiveClient());
  }

  /// The server must survive a client leaving and keep listening, since that is
  /// the documented behaviour and the teardown rewrite touches the same slot.
  void serverKeepsListeningAfterAClientLeaves() {
    WebSocketServer server(kEphemeral);
    QVERIFY(server.start());

    QWebSocket first;
    QVERIFY(attachClient(server, first));

    QSignalSpy gone(&server, &WebSocketServer::clientDisconnected);
    first.close();
    QVERIFY2(gone.wait(5000), "server never observed the client leaving");

    QVERIFY(server.isListening());
    QVERIFY(!server.hasActiveClient());

    QWebSocket second;
    QVERIFY2(attachClient(server, second), "server did not accept a reconnect");
    server.stop();
  }
};

QTEST_MAIN(TestWebSocketTeardown)
#include "test_websocket_teardown.moc"
