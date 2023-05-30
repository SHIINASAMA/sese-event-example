#include "sese/event/Event.h"

#include <atomic>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <thread>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

int setNonblocking(int fd) {
    auto option = fcntl(fd, F_GETFL);
    if (option != -1) {
        return fcntl(fd, F_SETFL, option | O_NONBLOCK);
    } else {
        return -1;
    }
}

class EchoEvent : public sese::event::EventLoop {
public:
    void onAccept(int fd) override {
        if (0 == setNonblocking(fd)) {
            this->createEvent(fd, EVENT_READ, nullptr);
        } else {
            close(fd);
        }
    }

    void onRead(sese::event::BaseEvent *event) override {
        memset(buffer, 0, 1024);
        auto l = read(event->fd, buffer, 1024);
        if (l == -1) {
            if (errno == ENOTCONN) {
                return;
            } else {
                close(event->fd);
                freeEvent(event);
            }
        } else {
            this->size = l;
            event->events &= ~EVENT_READ; // 删除读事件
            event->events |= EVENT_WRITE; // 添加写事件
            setEvent(event);
        }
    }

    void onWrite(sese::event::BaseEvent *event) override {
        write(event->fd, buffer, size);
        ::shutdown(event->fd, SHUT_RDWR);
        close(event->fd);
        freeEvent(event);
    }

    void onClose(sese::event::BaseEvent *event) override {
        freeEvent(event);
    }

public:
    void loop() {
        while (run) {
            dispatch(1000);
        }
    }

    void shutdown() {
        run = false;
    }

protected:
    char buffer[1024]{};
    size_t size = 0;
    std::atomic_bool run{true};
};

int main() {
    sockaddr_in address{};
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);

    auto listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    assert(setNonblocking(listenSocket) == 0);
    assert(bind(listenSocket, (sockaddr *) &address, sizeof(address)) == 0);
    listen(listenSocket, 255);

    EchoEvent event;
    event.setListenFd(listenSocket);
    assert(event.init() == true);

    std::thread th([&event]() {
        event.loop();
    });

    getchar();

    event.shutdown();
    th.join();
    return 0;
}