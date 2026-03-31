#include <chrono>
#include <iostream>
#include <iox/unix_domain_socket.hpp>
#include <string>
#include <thread>

namespace {
constexpr char REQUEST_SOCKET_NAME[] = "test_socket_request";
constexpr char REPLY_SOCKET_NAME[]   = "test_socket_reply";

int errorCode(const iox::PosixIpcChannelError error) {
    return static_cast<int>(error);
}

iox::expected<iox::UnixDomainSocket, iox::PosixIpcChannelError> createSocket(
    const char* name, const iox::PosixIpcChannelSide side) {
    return iox::UnixDomainSocketBuilder()
        .name(iox::PosixIpcChannelName_t(iox::TruncateToCapacity, name))
        .channelSide(side)
        .maxMsgSize(1024U)
        .maxMsgNumber(10U)
        .create();
}

iox::expected<iox::UnixDomainSocket, iox::PosixIpcChannelError> createClientSocketWithRetry(
    const char* name, const uint32_t maxRetries = 50U,
    const std::chrono::milliseconds retryDelay = std::chrono::milliseconds(20U)) {
    for (uint32_t attempt = 0U; attempt < maxRetries; ++attempt) {
        auto socketResult = createSocket(name, iox::PosixIpcChannelSide::CLIENT);
        if (!socketResult.has_error()) {
            return socketResult;
        }
        std::this_thread::sleep_for(retryDelay);
    }

    return iox::err(iox::PosixIpcChannelError::NO_SUCH_CHANNEL);
}

bool cleanupSocket(const char* name) {
    auto cleanup = iox::UnixDomainSocket::unlinkIfExists(
        iox::UnixDomainSocket::UdsName_t(iox::TruncateToCapacity, name));
    if (cleanup.has_error()) {
        std::cerr << "cleanup failed for " << name << ", error=" << errorCode(cleanup.error())
                  << std::endl;
        return false;
    }
    return true;
}

void server() {
    auto requestServerResult = createSocket(REQUEST_SOCKET_NAME, iox::PosixIpcChannelSide::SERVER);
    if (requestServerResult.has_error()) {
        std::cerr << "request server create failed, error="
                  << errorCode(requestServerResult.error()) << std::endl;
        return;
    }

    auto requestServer = std::move(requestServerResult.value());
    auto request       = requestServer.timedReceive(iox::units::Duration::fromSeconds(5U));
    if (request.has_error()) {
        std::cerr << "request receive failed, error=" << errorCode(request.error()) << std::endl;
        return;
    }

    std::cout << "server received: " << request.value() << std::endl;

    auto replyClientResult = createClientSocketWithRetry(REPLY_SOCKET_NAME);
    if (replyClientResult.has_error()) {
        std::cerr << "reply client create failed, error=" << errorCode(replyClientResult.error())
                  << std::endl;
        return;
    }

    auto replyClient = std::move(replyClientResult.value());
    auto replySend   = replyClient.send("ack: " + request.value());
    if (replySend.has_error()) {
        std::cerr << "reply send failed, error=" << errorCode(replySend.error()) << std::endl;
    }
}

void client() {
    auto replyServerResult = createSocket(REPLY_SOCKET_NAME, iox::PosixIpcChannelSide::SERVER);
    if (replyServerResult.has_error()) {
        std::cerr << "reply server create failed, error=" << errorCode(replyServerResult.error())
                  << std::endl;
        return;
    }

    auto requestClientResult = createClientSocketWithRetry(REQUEST_SOCKET_NAME);
    if (requestClientResult.has_error()) {
        std::cerr << "request client create failed, error="
                  << errorCode(requestClientResult.error()) << std::endl;
        return;
    }

    auto requestClient = std::move(requestClientResult.value());
    auto sendResult    = requestClient.send("hello from client");
    if (sendResult.has_error()) {
        std::cerr << "client send failed, error=" << errorCode(sendResult.error()) << std::endl;
        return;
    }

    auto replyServer = std::move(replyServerResult.value());
    auto reply       = replyServer.timedReceive(iox::units::Duration::fromSeconds(5U));
    if (reply.has_error()) {
        std::cerr << "client receive failed, error=" << errorCode(reply.error()) << std::endl;
        return;
    }

    std::cout << "client received: " << reply.value() << std::endl;
}
}  // namespace

int main() {
    if (!cleanupSocket(REQUEST_SOCKET_NAME) || !cleanupSocket(REPLY_SOCKET_NAME)) {
        return 1;
    }

    std::thread serverThread(server);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    client();
    serverThread.join();
    return 0;
}
