#ifndef OSP_NET_HPP__
#define OSP_NET_HPP__

#include <iostream>
#include <osp_parser.hpp>

#ifdef USE_ZMQ
#include <zmq.hpp>

void osp_net(OspParser *parser) {
    zmq::context_t context{1};
    zmq::socket_t socket{context, zmq::socket_type::rep};
    socket.bind("tcp://*:8001");

    std::string res;

    while (osp::running) {
        zmq::message_t request;

        // receive a request from client
        try {
            (void)socket.recv(request, zmq::recv_flags::none);

            std::cout << "Received " << request.to_string() << std::endl;

            res = parser->parse(request.to_string());

            // send the reply to the client
            socket.send(zmq::buffer(res), zmq::send_flags::none);

        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
    }
}
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h> // for sleep

void osp_net(OspParser *parser) {
    int s_sock, sock, num_read, num;
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    int sock_opt = 1;
    char msg[8192];
    std::string res;

    if ((s_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        return;
    }
#ifdef __APPLE__
    if (setsockopt(s_sock, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt))) {
        perror("setsockopt");
        return;
    }
#else
    if (setsockopt(s_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &sock_opt, sizeof(sock_opt))) {
        perror("setsockopt");
        return;
    }
#endif

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8001);

    if (::bind(s_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        return;
    }
    if (listen(s_sock, 4) < 0) {
        perror("listen");
        return;
    }
    while (osp::running || !osp::reset) {
        sock = accept(s_sock, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
        if (sock < 0) {
            perror("accept");
            break;
        }

        num_read = recv(sock, msg, sizeof(msg), 0);
        if (num_read <= 0) {
            res = "FAILED";
        } else {
            // wait .1 ms then read again to see if there was more data.
            usleep(100);
            num = recv(sock, &(msg[num_read]), sizeof(msg) - num_read, MSG_DONTWAIT);
            if (num > 0)
                num_read += num;
            msg[num_read] = 0;

            // std::cout << "Read " << valread << "bytes" << std::endl;
            std::cout << msg << std::endl;
            try {
                res = parser->parse(std::string(msg));
            } catch (...) {
                std::cout << "Parsing of JSON string failed." << std::endl;
                res = "FAILED";
            }
        }
        // std::cout <<"Sending: " << res << std::endl;
        send(sock, res.c_str(), res.length(), 0);
        close(sock);
    }
    close(s_sock);
}

#endif // USE_ZMQ

#endif // OSP_NET_HPP__