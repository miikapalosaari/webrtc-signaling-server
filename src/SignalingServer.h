#pragma once

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
};