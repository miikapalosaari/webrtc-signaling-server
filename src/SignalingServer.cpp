#include "SignalingServer.h"
#include <rtc/rtc.h>
#include <iostream>
#include <string>

SignalingServer* SignalingServer::s_instance = nullptr;

void SignalingServer::start(int port) {
    s_instance = this;

    rtcInitLogger(RTC_LOG_INFO, nullptr);

    rtcWsServerConfiguration cfg{};
    cfg.port = port;

    rtcCreateWebSocketServer(&cfg, onClientCallback);

    std::cout << "Listening on port " << port << std::endl;
}

void SignalingServer::onClientCallback(int server, int ws, void*) {
    if (s_instance) {
        s_instance->onClient(server, ws);
    }
}

void SignalingServer::onOpenCallback(int ws, void* ptr) {
    auto* self = static_cast<SignalingServer*>(ptr);
    if (self) {
        self->onOpen(ws);
    }
}

void SignalingServer::onMessageCallback(int ws, const char* msg, int size, void* ptr) {
    auto* self = static_cast<SignalingServer*>(ptr);
    if (self) {
        self->onMessage(ws, msg, size);
    }
}

void SignalingServer::onClosedCallback(int ws, void* ptr) {
    auto* self = static_cast<SignalingServer*>(ptr);
    if (self) {
        self->onClosed(ws);
    }
}

void SignalingServer::onClient(int /*server*/, int ws) {
    std::cout << "Client connected! Socket = " << ws << std::endl;

    Client client;
    client.socket = ws;
    m_clients.emplace(ws, std::move(client));

    rtcSetUserPointer(ws, this);

    rtcSetOpenCallback(ws, onOpenCallback);
    rtcSetMessageCallback(ws, onMessageCallback);
    rtcSetClosedCallback(ws, onClosedCallback);
}

void SignalingServer::onOpen(int ws) {
    std::cout << "Socket " << ws << " opened" << std::endl;
}

void SignalingServer::onMessage(int ws, const char* msg, int size) {
    if (!msg) {
        std::cout << "Null message" << std::endl;
        return;
    }

    if (size < 0) {
        size = -size;
    }

    try {
        json message = json::parse(msg, msg + size);
        std::string type = message.value("type", "");

        if (type == "register") {
            handleRegister(ws, message);
        }
        else if (type == "createRoom") {
            handleCreateRoom(ws, message);
        }
        else if (type == "joinRoom") {
            handleJoinRoom(ws, message);
        }
        else if (type == "leaveRoom") {
            handleLeaveRoom(ws, message);
        }
        else if (type == "offer") {
            handleOffer(ws, message);
        }
        else if (type == "answer") {
            handleAnswer(ws, message);
        }
        else if (type == "candidate") {
            handleCandidate(ws, message);
        }
        else {
            std::cout << "Unknown message type: " << type << std::endl;
        }
    }
    catch (const json::exception& e) {
        std::cout << "Invalid JSON: " << e.what() << std::endl;
    }
}

void SignalingServer::onClosed(int ws) {
    std::cout << "Socket " << ws << " closed" << std::endl;
    m_clients.erase(ws);
}

void SignalingServer::handleRegister(int ws, const json& message) {
    auto it = m_clients.find(ws);
    if (it == m_clients.end()) {
        return;
    }

    it->second.playerId = message.value("playerId", "");
    std::cout << "Registered player " << it->second.playerId << " on socket " << ws << std::endl;
}

void SignalingServer::handleCreateRoom(int ws, const json& message) {

}

void SignalingServer::handleJoinRoom(int ws, const json& message) {

}

void SignalingServer::handleLeaveRoom(int ws, const json& message) {

}

void SignalingServer::handleOffer(int ws, const json& message) {

}

void SignalingServer::handleAnswer(int ws, const json& message) {

}

void SignalingServer::handleCandidate(int ws, const json& message) {

}