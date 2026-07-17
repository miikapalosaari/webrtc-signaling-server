#include "SignalingServer.h"
#include <rtc/rtc.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>

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
        else if (type == "listRooms") {
            handleListRooms(ws, message);
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
    json dummy;
    handleLeaveRoom(ws, dummy);

    Client* client = findClient(ws);
    if (client) {
        std::cout << client->playerId << " disconnected" << std::endl;
    }
    m_clients.erase(ws);
}

void SignalingServer::handleRegister(int ws, const json& message) {
    Client* client = findClient(ws);
    if (!client) {
        return;
    }

    std::string playerId = message.value("playerId", "");
    if (playerId.empty()) {
        sendError(ws, "Player ID cannot be empty.");
        return;
    }

    if (findClientByPlayerId(playerId)) {
        sendError(ws, "Player ID already exists.");
        return;
    }

    client->playerId = playerId;

    json reply;
    reply["type"] = "registered";
    reply["playerId"] = playerId;

    sendJson(ws, reply);
}

void SignalingServer::handleListRooms(int ws, const json& message) {
    json reply;
    reply["type"] = "roomList";
    reply["rooms"] = json::array();

    for (const auto& [id, room] : m_rooms) {
        json roomInfo;
        roomInfo["id"] = room.id;
        roomInfo["name"] = room.name;
        roomInfo["players"] = room.playerSockets.size();
        roomInfo["maxPlayers"] = room.MAX_PLAYERS;

        reply["rooms"].push_back(roomInfo);
    }

    sendJson(ws, reply);
}

std::string SignalingServer::generateRoomId() {
    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        seeded = true;
    }

    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    while (true) {
        std::string roomId;

        for (int i = 0; i < 6; i++) {
            roomId += chars[std::rand() % chars.size()];
        }

        if (m_rooms.find(roomId) == m_rooms.end()) {
            return roomId;
        }
    }
}

void SignalingServer::handleCreateRoom(int ws, const json& message) {
    Client* client = findClient(ws);

    if (!client) {
        sendError(ws, "Unknown client.");
        return;
    }

    if (client->playerId.empty()) {
        sendError(ws, "Register first.");
        return;
    }

    if (!client->roomId.empty()) {
        sendError(ws, "Already in a room.");
        return;
    }

    Room room;
    room.id = generateRoomId();
    room.playerSockets.push_back(ws);
    room.hostSocket = ws;
    room.name = message.value("name", "New Room");

    client->roomId = room.id;

    m_rooms.emplace(room.id, room);

    json reply;
    reply["type"] = "roomCreated";
    reply["roomId"] = room.id;

    sendJson(ws, reply);
    std::cout << client->playerId << " created room " << room.id << std::endl;
}

void SignalingServer::handleJoinRoom(int ws, const json& message) {
    Client* client = findClient(ws);

    if (!client) {
        sendError(ws, "Unknown client.");
        return;
    }

    if (client->playerId.empty()) {
        sendError(ws, "Register first.");
        return;
    }

    if (!client->roomId.empty()) {
        sendError(ws, "Already in a room.");
        return;
    }

    std::string roomId = message.value("roomId", "");
    Room* room = findRoom(roomId);
    if (!room) {
        sendError(ws, "Room does not exist.");
        return;
    }

    if (room->playerSockets.size() >= room->MAX_PLAYERS) {
        sendError(ws, "Room is full.");
        return;
    }

    room->playerSockets.push_back(ws);
    client->roomId = roomId;

    json joined;
    joined["type"] = "playerJoined";
    joined["playerId"] = client->playerId;
    sendToRoomExcept(*room, joined, ws);

    json reply;
    reply["type"] = "roomJoined";
    reply["roomId"] = roomId;
    reply["players"] = json::array();

    for (int socket : room->playerSockets) {
        Client* other = findClient(socket);
        if (other) {
            reply["players"].push_back(other->playerId);
        }
    }

    sendJson(ws, reply);
    std::cout << client->playerId << " joined room " << roomId<< std::endl;
}

void SignalingServer::handleLeaveRoom(int ws, const json& message) {
    Client* client = findClient(ws);
    if (!client) {
        return;
    }

    Room* room = findRoom(client->roomId);
    if (!room) {
        client->roomId.clear();
        return;
    }

    json left;
    left["type"] = "playerLeft";
    left["playerId"] = client->playerId;
    sendToRoomExcept(*room, left, ws);

    auto& playerSockets = room->playerSockets;

    playerSockets.erase(std::remove(playerSockets.begin(), playerSockets.end(), ws), playerSockets.end());

    if (playerSockets.empty()) {
        m_rooms.erase(room->id);
    }

    client->roomId.clear();
    std::cout << client->playerId << " left room" << std::endl;
}

void SignalingServer::handleOffer(int ws, const json& message) {
    std::string target = message.value("target", "");
    Client* sender = findClient(ws);
    Client* receiver = findClientByPlayerId(target);

    if (!sender || !receiver) {
        sendError(ws, "Player not found.");
        return;
    }

    if (sender->roomId != receiver->roomId) {
        sendError(ws, "Players are not in the same room.");
        return;
    }
    
    json relay = message;
    relay["playerId"] = sender->playerId;
    sendJson(receiver->socket, relay);
}

void SignalingServer::handleAnswer(int ws, const json& message) {
    std::string target = message.value("target", "");
    Client* sender = findClient(ws);
    Client* receiver = findClientByPlayerId(target);

    if (!sender || !receiver) {
        sendError(ws, "Player not found.");
        return;
    }

    if (sender->roomId != receiver->roomId) {
        sendError(ws, "Players are not in the same room.");
        return;
    }

    json relay = message;
    relay["playerId"] = sender->playerId;
    sendJson(receiver->socket, relay);
}

void SignalingServer::handleCandidate(int ws, const json& message) {
    std::string target = message.value("target", "");
    Client* sender = findClient(ws);
    Client* receiver = findClientByPlayerId(target);

    if (!sender || !receiver) {
        sendError(ws, "Player not found.");
        return;
    }

    if (sender->roomId != receiver->roomId) {
        sendError(ws, "Players are not in the same room.");
        return;
    }

    json relay = message;
    relay["playerId"] = sender->playerId;
    sendJson(receiver->socket, relay);
}

Client* SignalingServer::findClient(int socket) {
    auto it = m_clients.find(socket);
    if (it == m_clients.end()) {
        return nullptr;
    }
    return &it->second;
}

Client* SignalingServer::findClientByPlayerId(const std::string& playerId) {
    for (auto& [socket, client] : m_clients) {
        if (client.playerId == playerId) {
            return &client;
        }
    }
    return nullptr;
}

Room* SignalingServer::findRoom(const std::string& roomId) {
    auto it = m_rooms.find(roomId);
    if (it == m_rooms.end()) {
        return nullptr;
    }
    return &it->second;
}

void SignalingServer::sendJson(int socket, const json& message) {
    std::string text = message.dump();
    rtcSendMessage(socket, text.c_str(), -1);
}

void SignalingServer::sendError(int socket, const std::string& error) {
    json message;
    message["type"] = "error";
    message["message"] = error;
    sendJson(socket, message);
}

void SignalingServer::sendToRoom(const Room& room, const json& message) {
    for (int socket : room.playerSockets) {
        sendJson(socket, message);
    }
}

void SignalingServer::sendToRoomExcept(const Room& room, const json& message, int exceptSocket) {
    for (int socket : room.playerSockets) {
        if (socket == exceptSocket) {
            continue;
        }
        sendJson(socket, message);
    }
}