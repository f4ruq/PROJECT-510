#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

const std::string sentinel_code = "__::R7g!zPq$w9__";
std::mutex globalMutex;
std::atomic<bool> exit_check{false};

void clear_terminal(){std::cout << "\033[2J\033[H";}

void clear_line(){std::cout << "\033[F\033[2K";}

// handles user input and updates the shared response pointer
void user_input(std::string*& response_ptr) 
{
    {//mutex scope
        std::lock_guard<std::mutex> lock(globalMutex);
        std::string connect_message = "client is connected.";
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
void main_system_func(zmq::socket_t& socket_, zmq::context_t& context, std::string*& response_ptr)
{
    while(true)
    {
        if(exit_check)
        {
            std::string exit_message = "client left the chat.";
            zmq::message_t request(exit_message.size());
            memcpy(request.data(), exit_message.data(), exit_message.size());
            socket_.send(request, zmq::send_flags::none);
            socket_.close();
            context.shutdown();
            context.close();
            break;
        }

        // poll for incoming server messages
        zmq::pollitem_t items[] = {
            { static_cast<void*>(socket_), 0, ZMQ_POLLIN, 0 }
        };

        zmq::poll(&items[0], 1, std::chrono::milliseconds(100));

        if(items[0].revents & ZMQ_POLLIN)
        {
            zmq::message_t reply;
            socket_.recv(reply, zmq::recv_flags::none);
            std::string received = reply.to_string();
            
            if(received != sentinel_code)
            {
                std::cout << "message from server: " << received << std::endl;
            }
        }

        //send the latest user input to the server
        std::string response_copy;
        {//mutex scope
            std::lock_guard<std::mutex> lock(globalMutex);
            response_copy = *response_ptr;
            *response_ptr = sentinel_code;
        }//
        zmq::message_t request(response_copy.size());
        memcpy(request.data(), response_copy.data(), response_copy.size());
        socket_.send(request, zmq::send_flags::none);
    }
}

//optional function for manual sending
void send_message(std::string& response_, zmq::socket_t& socket_, zmq::context_t& context)
{
    while(true)
    {
        std::cout << "type your message: ";
        std::getline(std::cin, response_);

        if (response_ != "exit")
        {
            zmq::message_t request(response_.size());
            memcpy(request.data(), response_.data(), response_.size());

            socket_.send(request, zmq::send_flags::none);
        }
        else
        {
            std::string exit_message = "client left the chat.";
            zmq::message_t request(exit_message.size());
            memcpy(request.data(), exit_message.data(), exit_message.size());
            socket_.send(request, zmq::send_flags::none);
            socket_.close();
            context.shutdown();
            context.close();
            break;
        }
    }     
}

//optional function for receiving messages outside main thread
void receive_message(zmq::message_t& request_, zmq::socket_t& socket_)
{   
    while(true)
    {     
        zmq::recv_result_t result = socket_.recv(request_, zmq::recv_flags::none);
            
        if(result){
            std::string received_message = request_.to_string();
            std::cout << "message from server: " << received_message << std::endl;
        }
        else 
        {
            std::cout << "can't connect to server." << std::endl;
        }
    }
}

//used for idle echoing/pinging if needed
void idling(std::string &idle_code, zmq::socket_t &socket, zmq::message_t& request_)
{
    while(true)
    {
        zmq::message_t idle(idle_code.size());
        memcpy(idle.data(), idle_code.data(), idle_code.size());
        socket.send(idle, zmq::send_flags::none);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        zmq::recv_result_t result = socket.recv(request_, zmq::recv_flags::none);
            
        if(!result){
            std::cout << "can't connect to server." << std::endl;
        }
    }
}


int main()
{
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::dealer);
    socket.setsockopt(ZMQ_LINGER, 0);
    socket.connect("tcp://localhost:5555");

    std::string response = sentinel_code;
    std::string* response_ptr = &response;

    std::thread t_input(user_input, std::ref(response_ptr));
    std::thread t_comm(main_system_func, std::ref(socket), std::ref(context), std::ref(response_ptr));

    t_input.join();
    t_comm.join();

    return 0;
}