#pragma once

class StubServer {
public:
    explicit StubServer(int port);
    bool run();

private:
    int port_;
};
