#include "SignalingServer.h"

#include <chrono>
#include <thread>

int main() {
    SignalingServer server;

    server.start(25565);

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}