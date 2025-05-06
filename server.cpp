#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

const std::string sentinel_code = "__::R7g!zPq$w9__";
std::mutex globalMutex;
std::string client_id_str;
std::atomic<bool> exit_check{false};

void clear_terminal(){std::cout << "\033[2J\033[H";}

void clear_line(){std::cout << "\033[F\033[2K";}

// handles user input and updates the shared response pointer
void user_input(std::string*& response_ptr) 
{
    {//mutex scope
        std::lock_guard<std::mutex> lock(globalMutex);
        std::string connect_message = "server is connected.";
        *response_ptr = connect_message;
    }//

    while (true)   
    {
        std::string response;
        std::getline(std::cin, response); clear_line();
        std::cout << "you: " << response << std::endl;
        
        if(response == "exit")
        {
            exit_check = true;
            break;
        }

        {//mutex scope
            std::lock_guard<std::mutex> lock(globalMutex);
            *response_ptr = response;
        }//
    }
}

// main loop; receives messages, checks input, sends messages
void main_system_func(zmq::message_t& request_, zmq::message_t& identity_, zmq::socket_t& socket_, std::string*& response_ptr, zmq::context_t& context) 
{
    while (true) 
    {
        if(exit_check)
        {
            std::string exit_message = "server left the chat.";
            zmq::message_t id_msg(client_id_str.begin(), client_id_str.end());
            zmq::message_t reply(exit_message.size());
            memcpy(reply.data(), exit_message.data(), exit_message.size());
            socket_.send(id_msg, zmq::send_flags::sndmore);
            socket_.send(reply, zmq::send_flags::none);
            socket_.close();
            context.shutdown();
            context.close();
            break;
        }
        
        // poll the socket for new incoming data with a 100ms timeout
        zmq::pollitem_t items[] = {{static_cast<void*>(socket_), 0, ZMQ_POLLIN, 0}};
        zmq::poll(items, 1, 100);

        if (items[0].revents & ZMQ_POLLIN) 
        {
            //router socket requires receiving the client identity first
            socket_.recv(identity_, zmq::recv_flags::none);
            socket_.recv(request_, zmq::recv_flags::none);
            client_id_str = identity_.to_string();

            std::string received_message = request_.to_string();
            
            if (received_message != sentinel_code) 
            {
                std::cout << "message from client: " << received_message << std::endl;
            }
        }

        {//mutex scope
            std::lock_guard<std::mutex> lock(globalMutex);
            std::string response_ = *response_ptr;

            if (response_ != sentinel_code && !client_id_str.empty()) 
            {
                zmq::message_t id_msg(client_id_str.begin(), client_id_str.end());
                zmq::message_t reply(response_.size());
                memcpy(reply.data(), response_.data(), response_.size());
                socket_.send(id_msg, zmq::send_flags::sndmore);
                socket_.send(reply, zmq::send_flags::none);
                *response_ptr = sentinel_code;
            }
              
        }//
    }
}

//used for idle echoing/pinging if needed
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

//optional function for manual send
void send_message(std::string& response_, zmq::message_t& identity_, zmq::socket_t& socket_)
{
    while(true)
    {
        std::getline(std::cin,response_);
        
        if(response_ != "exit")
        {
            zmq::message_t reply(response_.size());
            memcpy(reply.data(), response_.data(), response_.size());
            // send mesage
            socket_.send(identity_, zmq::send_flags::sndmore); // Önce istemcinin kimliğini gönder
            socket_.send(reply, zmq::send_flags::none); // Sonra mesajı gönder   
        } 
        else
        {
            std::string exit_message = "server left the chat";
            zmq::message_t reply(exit_message.size());
            memcpy(reply.data(), exit_message.data(), exit_message.size());
            socket_.send(identity_, zmq::send_flags::sndmore); // Önce istemcinin kimliğini gönder
            socket_.send(reply, zmq::send_flags::none); // Sonra mesajı gönder 
            break;
        }
    }
}

//optional function for receiving messages outside main thread
void receive_message(zmq::message_t& request_, zmq::message_t& identity_, zmq::socket_t& socket_)
{
    while(true)
    {    
        socket_.recv(identity_, zmq::recv_flags::none);
        zmq::recv_result_t result = socket_.recv(request_, zmq::recv_flags::none);
        if(result)
        {
            std::string received_message = request_.to_string();
            std::cout << "message from client: " << received_message << std::endl;
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
    std::string response = sentinel_code;
    std::string* response_ptr = &response;

    std::thread user_inputThread(user_input, std::ref(response_ptr));
    std::thread main_system_funcThread(main_system_func, std::ref(request), std::ref(identity), std::ref(socket), std::ref(response_ptr), std::ref(context));

    user_inputThread.join();
    main_system_funcThread.join();
    return 0;
}
