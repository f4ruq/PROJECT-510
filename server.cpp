#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>

std::mutex globalMutex;
std::string client_id_str;

void user_input(std::string*& response_ptr) 
{
    while (true)   
    {
        std::string response;
        //std::cout << "type your message: " << std::endl;
        std::getline(std::cin, response);
        
        {/////////////////--- MUTEX SCOPE ---////////////////////////////
            std::lock_guard<std::mutex> lock(globalMutex);
            *response_ptr = response;
        }/////////////////--- MUTEX SCOPE ---////////////////////////////
    }
}

void main_system_func(zmq::message_t& request_, zmq::message_t& identity_, zmq::socket_t& socket_, std::string*& response_ptr) 
{
    while (true) {
        zmq::pollitem_t items[] = {{static_cast<void*>(socket_), 0, ZMQ_POLLIN, 0}};
        zmq::poll(items, 1, 100);

        if (items[0].revents & ZMQ_POLLIN) 
        {
            socket_.recv(identity_, zmq::recv_flags::none);
            socket_.recv(request_, zmq::recv_flags::none);
            client_id_str = identity_.to_string();

            std::string received_message = request_.to_string();
            
            if (received_message != "15710xdqwe") 
            {
                std::cout << "Client mesaji: " << received_message << std::endl;
            }
        }

        {/////////////////--- MUTEX SCOPE ---////////////////////////////
            std::lock_guard<std::mutex> lock(globalMutex);
            std::string response_ = *response_ptr;

            if (response_ != "15710xdqwe" && !client_id_str.empty()) 
            {
                zmq::message_t id_msg(client_id_str.begin(), client_id_str.end());
                zmq::message_t reply(response_.size());
                memcpy(reply.data(), response_.data(), response_.size());
                socket_.send(id_msg, zmq::send_flags::sndmore);
                socket_.send(reply, zmq::send_flags::none);
                *response_ptr = "15710xdqwe";
            }
        }/////////////////--- MUTEX SCOPE ---////////////////////////////
    }
}

void idling(std::string &idle_code, zmq::socket_t &socket, zmq::message_t& identity, zmq::message_t& request_)
{
    while(true)
    {
        socket.recv(identity, zmq::recv_flags::none);
        zmq::recv_result_t result = socket.recv(request_, zmq::recv_flags::none);
         
        if(!result)
        {
            std::cout << "can't connect to client." << std::endl;
        }

        zmq::message_t idle(idle_code.size());
        memcpy(idle.data(), idle_code.data(), idle_code.size());

        socket.send(identity, zmq::send_flags::sndmore);
        socket.send(idle, zmq::send_flags::none);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void send_message(std::string& response_, zmq::message_t& identity_, zmq::socket_t& socket_)
{
    while(true){
        //std::cout<< "type your message: ";
        std::getline(std::cin,response_);
        
        if(response_ != "exit"){
            zmq::message_t reply(response_.size());
            memcpy(reply.data(), response_.data(), response_.size());
            // send mesage
            socket_.send(identity_, zmq::send_flags::sndmore); // Önce istemcinin kimliğini gönder
            socket_.send(reply, zmq::send_flags::none); // Sonra mesajı gönder   
        } 
        else{
            std::string exit_message = "server konusmadan ayrildi.";
            zmq::message_t reply(exit_message.size());
            memcpy(reply.data(), exit_message.data(), exit_message.size());
            socket_.send(identity_, zmq::send_flags::sndmore); // Önce istemcinin kimliğini gönder
            socket_.send(reply, zmq::send_flags::none); // Sonra mesajı gönder 
            break;
        }
    }
}

void receive_message(zmq::message_t& request_, zmq::message_t& identity_, zmq::socket_t& socket_)
{
    while(true){    
        socket_.recv(identity_, zmq::recv_flags::none);
        zmq::recv_result_t result = socket_.recv(request_, zmq::recv_flags::none);
            
        if(result)
        {
            std::string received_message = request_.to_string();
            std::cout << "client mesaji: " << received_message << std::endl;
        }
    }
}

int main() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::router);
    socket.setsockopt(ZMQ_LINGER, 0);
    socket.bind("tcp://*:5555");

    zmq::message_t identity;
    zmq::message_t request;
    std::string response = "15710xdqwe";
    std::string* response_ptr = &response;

    std::thread user_inputThread(user_input, std::ref(response_ptr));
    std::thread main_system_funcThread(main_system_func, std::ref(request), std::ref(identity), std::ref(socket), std::ref(response_ptr));

    user_inputThread.join();
    main_system_funcThread.join();
    return 0;
}
