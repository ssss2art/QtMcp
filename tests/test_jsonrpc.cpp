// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QString>

#include <nlohmann/json.hpp>

#include "transport/jsonrpc_handler.h"

using json = nlohmann::json;

class JsonRpcTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Ensure QCoreApplication exists for Qt functionality
    if (QCoreApplication::instance() == nullptr) {
      int argc = 0;
      char** argv = nullptr;
      app_ = new QCoreApplication(argc, argv);
    }
    handler_ = std::make_unique<qtmcp::JsonRpcHandler>();
  }

  void TearDown() override {
    handler_.reset();
    // Don't delete app_ - it persists across tests
  }

  std::unique_ptr<qtmcp::JsonRpcHandler> handler_;
  static QCoreApplication* app_;
};

QCoreApplication* JsonRpcTest::app_ = nullptr;

TEST_F(JsonRpcTest, PingReturnsong) {
  QString request = R"({"jsonrpc":"2.0","id":1,"method":"ping"})";
  QString response = handler_->HandleMessage(request);

  json resp = json::parse(response.toStdString());
  EXPECT_EQ(resp["jsonrpc"], "2.0");
  EXPECT_EQ(resp["id"], 1);
  EXPECT_EQ(resp["result"], "pong");
}

TEST_F(JsonRpcTest, GetVersionReturnsVersionInfo) {
  QString request = R"({"jsonrpc":"2.0","id":2,"method":"getVersion"})";
  QString response = handler_->HandleMessage(request);

  json resp = json::parse(response.toStdString());
  EXPECT_EQ(resp["jsonrpc"], "2.0");
  EXPECT_EQ(resp["id"], 2);
  EXPECT_TRUE(resp["result"].contains("version"));
  EXPECT_TRUE(resp["result"].contains("protocol"));
  EXPECT_TRUE(resp["result"].contains("name"));
  EXPECT_EQ(resp["result"]["name"], "QtMCP");
}

TEST_F(JsonRpcTest, EchoReturnsParams) {
  QString request = R"({"jsonrpc":"2.0","id":3,"method":"echo","params":{"foo":"bar"}})";
  QString response = handler_->HandleMessage(request);

  json resp = json::parse(response.toStdString());
  EXPECT_EQ(resp["jsonrpc"], "2.0");
  EXPECT_EQ(resp["id"], 3);
  EXPECT_EQ(resp["result"]["foo"], "bar");
}

TEST_F(JsonRpcTest, UnknownMethodReturnsError) {
  QString request = R"({"jsonrpc":"2.0","id":4,"method":"unknownMethod"})";
  QString response = handler_->HandleMessage(request);

  json resp = json::parse(response.toStdString());
  EXPECT_EQ(resp["jsonrpc"], "2.0");
  EXPECT_EQ(resp["id"], 4);
  EXPECT_TRUE(resp.contains("error"));
  EXPECT_EQ(resp["error"]["code"], -32601);  // Method not found
}

TEST_F(JsonRpcTest, InvalidJsonReturnsParseError) {
  QString request = R"({invalid json)";
  QString response = handler_->HandleMessage(request);

  json resp = json::parse(response.toStdString());
  EXPECT_TRUE(resp.contains("error"));
  EXPECT_EQ(resp["error"]["code"], -32700);  // Parse error
}

TEST_F(JsonRpcTest, MissingJsonRpcVersionReturnsError) {
  QString request = R"({"id":1,"method":"ping"})";
  QString response = handler_->HandleMessage(request);

  json resp = json::parse(response.toStdString());
  EXPECT_TRUE(resp.contains("error"));
  EXPECT_EQ(resp["error"]["code"], -32600);  // Invalid request
}

TEST_F(JsonRpcTest, NotificationReturnsEmptyResponse) {
  // Notification has no id
  QString request = R"({"jsonrpc":"2.0","method":"ping"})";
  QString response = handler_->HandleMessage(request);

  EXPECT_TRUE(response.isEmpty());
}

TEST_F(JsonRpcTest, StringIdIsPreserved) {
  QString request = R"({"jsonrpc":"2.0","id":"my-request-id","method":"ping"})";
  QString response = handler_->HandleMessage(request);

  json resp = json::parse(response.toStdString());
  EXPECT_EQ(resp["id"], "my-request-id");
}

TEST_F(JsonRpcTest, GetModesReturnsArray) {
  QString request = R"({"jsonrpc":"2.0","id":5,"method":"getModes"})";
  QString response = handler_->HandleMessage(request);

  json resp = json::parse(response.toStdString());
  EXPECT_EQ(resp["jsonrpc"], "2.0");
  EXPECT_TRUE(resp["result"].is_array());
  EXPECT_EQ(resp["result"].size(), 3);
}

TEST_F(JsonRpcTest, CustomMethodCanBeRegistered) {
  handler_->RegisterMethod("customMethod", [](const QString& params) -> QString {
    return R"({"custom":"response"})";
  });

  QString request = R"({"jsonrpc":"2.0","id":6,"method":"customMethod"})";
  QString response = handler_->HandleMessage(request);

  json resp = json::parse(response.toStdString());
  EXPECT_EQ(resp["result"]["custom"], "response");
}
