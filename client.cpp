#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>

std::mutex globalMutex;
std::atomic<bool> exit_check{false};

void clear_terminal(){std::cout << "\033[2J\033[H";}

void user_input(std::string*& response_ptr)
{
    while(true)
    {
        std::string response;
        //std::cout << "type your message: " << std::endl;
        std::getline(std::cin, response);
        if(response == "exit")
        {
            exit_check = true;
            break;
        }
        {/////////////////--- MUTEX SCOPE ---////////////////////////////
            std::lock_guard<std::mutex> lock(globalMutex);
            *response_ptr = response;
        }/////////////////--- MUTEX SCOPE ---////////////////////////////
    }
}

void main_system_func(zmq::socket_t& socket_, zmq::context_t& context, std::string*& response_ptr)
{
    std::string first_message = "client is connected.";
    zmq::message_t request(first_message.size());
    memcpy(request.data(), first_message.data(), first_message.size());
    socket_.send(request, zmq::send_flags::none);

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

        zmq::pollitem_t items[] = {
            { static_cast<void*>(socket_), 0, ZMQ_POLLIN, 0 }
        };

        zmq::poll(&items[0], 1, std::chrono::milliseconds(100));

        if(items[0].revents & ZMQ_POLLIN)
        {
            zmq::message_t reply;
            socket_.recv(reply, zmq::recv_flags::none);
            std::string received = reply.to_string();
            
            if(received != "15710xdqwe")
            {
                std::cout << "server mesaji: " << received << std::endl;
            }
        }

        std::string response_copy;
        {/////////////////--- MUTEX SCOPE ---////////////////////////////
            std::lock_guard<std::mutex> lock(globalMutex);
            response_copy = *response_ptr;
            *response_ptr = "15710xdqwe";
        }/////////////////--- MUTEX SCOPE ---////////////////////////////
        zmq::message_t request(response_copy.size());
        memcpy(request.data(), response_copy.data(), response_copy.size());
        socket_.send(request, zmq::send_flags::none);
    }
}

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
            std::string exit_message = "client konusmadan ayrildi.";
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

void receive_message(zmq::message_t& request_, zmq::socket_t& socket_)
{   
    while(true)
    {     
        zmq::recv_result_t result = socket_.recv(request_, zmq::recv_flags::none);
            
        if(result){
            std::string received_message = request_.to_string();
            std::cout << "server mesaji: " << received_message << std::endl;
        }
        else 
        {
            std::cout << "mesaj alinamadi." << std::endl;
        }
    }
}

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

    std::string response = "15710xdqwe";
    std::string* response_ptr = &response;

    std::thread t_input(user_input, std::ref(response_ptr));
    std::thread t_comm(main_system_func, std::ref(socket), std::ref(context), std::ref(response_ptr));

    t_input.join();
    t_comm.join();

    return 0;
}
