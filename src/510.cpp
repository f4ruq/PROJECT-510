#include "510.hpp"

std::mutex globalMutex;
std::atomic<bool> server_exit_check{0};
std::atomic<bool> client_exit_check{0};
std::atomic<bool> server_socket_active{0};
std::atomic<bool> client_socket_active{0};
std::string client_id_str;
std::vector<std::string> message_log;
std::string response = sentinel_code;
std::string* response_ptr = &response;
std::thread zmq_client_funcThread;
std::thread zmq_server_funcThread;

void zmq_client_func(zmq::socket_t& socket_, zmq::context_t& context, std::string*& response_ptr)
{
        while(client_socket_active)
        {
            if(client_exit_check)
            {
                std::string exit_message = "client left the chat.";
                zmq::message_t request(exit_message.size());
                memcpy(request.data(), exit_message.data(), exit_message.size());
                socket_.send(request, zmq::send_flags::none);
                //socket_.close();
                
                /*context.shutdown();
                context.close();
                */
                break;
            }

            // poll for incoming server messages
            zmq::pollitem_t items[] = {
                { static_cast<void*>(socket_), 0, ZMQ_POLLIN, 0 }
            };

            zmq::poll(&items[0], 1, std::chrono::milliseconds(100));
            if(client_exit_check){break;}

            if(items[0].revents & ZMQ_POLLIN)
            {
                zmq::message_t reply;
                socket_.recv(reply, zmq::recv_flags::none);
                std::string received = reply.to_string();
                
                if(received != sentinel_code)
                {
                    std::lock_guard<std::mutex> lock(globalMutex);
                    message_log.push_back("server: " + received);
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
            if(response_copy != sentinel_code){message_log.push_back("you: " + response_copy);}
            socket_.send(request, zmq::send_flags::none);
        }

}

void zmq_server_func(zmq::message_t& request_, zmq::message_t& identity_, zmq::socket_t& socket_, std::string*& response_ptr, 
    zmq::context_t& context, std::string*& received_message_ptr) 
{
    while (server_socket_active) 
        {
            if(server_exit_check)
            {
                std::string exit_message = "server left the chat.";
                zmq::message_t id_msg(client_id_str.begin(), client_id_str.end());
                zmq::message_t reply(exit_message.size());
                memcpy(reply.data(), exit_message.data(), exit_message.size());
                socket_.send(id_msg, zmq::send_flags::sndmore);
                socket_.send(reply, zmq::send_flags::none);
                //socket_.close();
                /*
                context.shutdown();
                context.close();
                */
                break;
            }
            
            // poll the socket for new incoming data with a 100ms timeout
            zmq::pollitem_t items[] = {{static_cast<void*>(socket_), 0, ZMQ_POLLIN, 0}};
            zmq::poll(items, 1, 100);
            if(server_exit_check){break;}
            if (items[0].revents & ZMQ_POLLIN) 
            {
                //router socket requires receiving the client identity first
                socket_.recv(identity_, zmq::recv_flags::none);
                socket_.recv(request_, zmq::recv_flags::none);
                client_id_str = identity_.to_string();

                std::string received_message = request_.to_string();
                
                if (received_message != sentinel_code) 
                {
                    std::lock_guard<std::mutex> lock(globalMutex);
                    received_message_ptr = &received_message;
                    message_log.push_back("client: " + received_message);
                    std::cout << "message from client: " << received_message << std::endl;
                }
            }

            {//mutex scope  
                std::lock_guard<std::mutex> lock(globalMutex);
                std::string response_ = *response_ptr;
                
                if (response_ != sentinel_code && !client_id_str.empty()) 
                {
                    message_log.push_back("you: " + response_);
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
