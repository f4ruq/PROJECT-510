#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>

void send_message(zmq::socket_t &socket){

    std::string requestMessage = "";
    zmq::message_t firstMessage(requestMessage.size());
    memcpy(firstMessage.data(), requestMessage.data(), requestMessage.size());
    socket.send(firstMessage, zmq::send_flags::none);

    while (true) {
        std::string message;
        std::cout << "mesaj gir: ";
        std::getline(std::cin, message);
        
        zmq::message_t request(message.size());
        memcpy(request.data(), message.data(), message.size());

        socket.send(request, zmq::send_flags::none);
    }
}

void receive_message(zmq::socket_t &socket){
    while(true){
        zmq::message_t replyFromServer;
        zmq::recv_result_t result = socket.recv(replyFromServer, zmq::recv_flags::none);
        
        if (!result) {
            std::cerr << "hata: sunucudan yanit alinamadi!" << std::endl;
            continue;
        }

        std::cout << "sunucudan gelen Cevap: " << replyFromServer.to_string() << std::endl;
    }
}

int main() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::dealer);
    socket.connect("tcp://localhost:5555");

    std::thread sendThread(send_message, std::ref(socket));
    std::thread receiveThread(receive_message, std::ref(socket));

    sendThread.join();
    receiveThread.join();

    return 0;
}

