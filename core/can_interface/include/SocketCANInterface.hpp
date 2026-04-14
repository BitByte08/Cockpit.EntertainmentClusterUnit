#ifndef CORE_SOCKETCANINTERFACE_CPP
#define CORE_SOCKETCANINTERFACE_CPP
#include "CANInterface.hpp"

class SocketCANInterface : public CANInterface {
    Q_OBJECT
public:
    explicit SocketCANInterface(const std::string &interface_name);
    ~SocketCANInterface() override;

    void start() override;
    void stop() override;
private:
    int socket_fd_{-1};
    bool running_{false};
};

#endif // CORE_SOCKETCANINTERFACE_CPP
