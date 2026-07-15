#pragma once
#include <iostream>
#include <unordered_map>
#include <vector>
#include <json.hpp>

struct Client {
    int socket = -1;
    std::string playerId;
    std::string roomId;
    bool ready = false;
};

struct Room {
    std::string id;
    std::vector<int> playerSockets;
    static constexpr int MAX_PLAYERS = 4;
};

using json = nlohmann::json;

class SignalingServer {
public:
    void start(int port);

private:
    static SignalingServer* s_instance;

    void onClient(int server, int ws);
    void onOpen(int ws);
    void onMessage(int ws, const char* msg, int size);
    void onClosed(int ws);

    static void onClientCallback(int server, int ws, void*);
    static void onOpenCallback(int ws, void* ptr);
    static void onMessageCallback(int ws, const char* msg, int size, void* ptr);
    static void onClosedCallback(int ws, void* ptr);

    void handleRegister(int ws, const json& message);

    void handleCreateRoom(int ws, const json& message);
    void handleJoinRoom(int ws, const json& message);
    void handleLeaveRoom(int ws, const json& message);

    void handleOffer(int ws, const json& message);
    void handleAnswer(int ws, const json& message);
    void handleCandidate(int ws, const json& message);

    std::string generateRoomId();

    Client* findClient(int socket);
    Client* findClientByPlayerId(const std::string& playerId);
    Room* findRoom(const std::string& roomId);
    void sendJson(int socket, const json& message);
    void sendError(int socket, const std::string& error);

    void sendToRoom(const Room& room, const json& message);
    void sendToRoomExcept(const Room& room, const json& message, int exceptSocket);

    std::unordered_map<int, Client> m_clients;
    std::unordered_map<std::string, Room> m_rooms;
};