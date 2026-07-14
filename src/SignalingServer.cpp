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

    std::string text(msg, static_cast<size_t>(size));

    std::cout << "Message from " << ws << ": " << text << std::endl;
}

void SignalingServer::onClosed(int ws) {
    std::cout << "Socket " << ws << " closed" << std::endl;
}