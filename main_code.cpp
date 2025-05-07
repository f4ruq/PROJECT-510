#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <fstream>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <mutex>
#include <thread>
#include <chrono>
#include <thread>

class customer
{
protected:
    std::string name, profession, password, clientMessage;
    int id;
    float balance, debt;
    int age;

public:
    customer(std::string name_, std::string profession_, std::string password_, int id_, float balance_, float debt_, int age_)
        : name(name_), profession(profession_), password(password_), id(id_), balance(balance_), debt(debt_), age(age_) {}

    void set_name(std::string name_) { name = name_; }
    void set_profession(std::string profession_) { profession = profession_; }
    int get_id() { return id; }
    void set_balance(float balance_) { balance = balance_; }
    void set_debt(float debt_) { debt = debt_; }
    void set_age(int age_) { age = age_; }
    std::string get_name() { return name; }
    std::string get_profession() { return profession; }
    float get_balance() { return balance; }
    float get_debt() { return debt; }
    int get_age() { return age; }
    void set_password(std::string password_) { password = password_; }
    std::string get_password() { return password; }

    void show_information() {
        std::cout << "name: " << name << std::endl << "profession: " << profession << std::endl;
        std::cout << "id: " << id << std::endl << "age: " << age << std::endl << "balance: " << balance << std::endl << "debt: " << debt << std::endl;
    }
};


const std::string sentinel_code = "__::R7g!zPq$w9__";
std::mutex globalMutex;
std::string client_id_str;
std::atomic<bool> exit_check{false};
std::vector<customer*> users;
int current_user_index = -1;
bool login_successful = false;

void clear_terminal(){std::cout << "\033[2J\033[H";}

void clear_line(){std::cout << "\033[F\033[2K";}

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

void messaging_func(zmq::message_t& request_, zmq::message_t& identity_, zmq::socket_t& socket_, std::string*& response_ptr, zmq::context_t& context) 
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
        /*
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
              
        }//*/
    }
}

void clearScreen() 
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void create_new_user() 
{
    std::string name, profession, password;
    int id, age;
    float balance, debt;
    id = rand() % 999 + 100;
    balance = 0;
    debt = 0;
    
    clearScreen();
    std::cout << "Please enter your name: "; std::cin >> name;
    std::cout << "Please enter your profession: "; std::cin >> profession;
    std::cout << "Please enter your password: "; std::cin >> password;
    std::cout << "Please enter your age: "; std::cin >> age;

    customer* new_user = new customer(name, profession, password, id, balance, debt, age);
    users.push_back(new_user);
    
    clearScreen();
    std::cout << "User registered successfully." << std::endl;
}

void txt_to_constructor() 
{
    std::string user_info, profession, name, password;
    int id, age; float balance, debt;
    int line_number = 0;
    int user_number = 1;
    
    std::ifstream user_document;
    user_document.open("user_info.txt");

    if (!user_document.is_open()) 
    {
        std::cout << "error: unable to open the file." << std::endl;
    }
    else 
    {
        while (getline(user_document, user_info)) 
        {
            
            if(line_number % 8 == 0) {name = user_info;}
            else if(line_number % 8 == 1) {password = user_info;}
            else if(line_number % 8 == 2) {id = stoi(user_info);}
            else if(line_number % 8 == 3) {profession = user_info;}
            else if(line_number % 8 == 4) {age = stoi(user_info);}
            else if(line_number % 8 == 5) {balance = stof(user_info);}
            else if(line_number % 8 == 6) {debt = stof(user_info); user_number++;
                
                customer* new_user = new customer(name, profession, password, id, balance, debt, age);
                users.push_back(new_user);
            }
            line_number++;
        }
        user_document.close();
    }
}

void constructor_to_txt() 
{
    std::ofstream user_document;
    int i = 0;
    user_document.open("user_info.txt");
    
    if(user_document.is_open()) {
        while(i < users.size()) {
            user_document << users[i]->get_name() << std::endl;
            user_document << users[i]->get_password() << std::endl;
            user_document << std::to_string(users[i]->get_id()) << std::endl;
            user_document << users[i]->get_profession() << std::endl;
            user_document << std::to_string(users[i]->get_age()) << std::endl;
            user_document << std::to_string(users[i]->get_balance()) << std::endl;
            user_document << std::to_string(users[i]->get_debt()) << std::endl << std::endl;
            i++;
        }
    }
    else {std::cout << "error: unable to open file" << std::endl;}
    user_document.close();
}

void display_users_from_document() 
{
    std::string user_info;
    int line_number = 0;
    int user_number = 1;
    
    std::ifstream user_document;
    user_document.open("user_info.txt");

    if (!user_document.is_open()) 
    {
        std::cout << "error: unable to open the file." << std::endl;
    }
    else 
    {
        while (getline(user_document, user_info)) 
        {
            if(line_number % 8 == 0) 
            {
                std::cout << "user (" << user_number << ")'s info: " << std::endl;
                std::cout << "name: " << user_info << std::endl;
            }
            else if(line_number % 8 == 1) 
            {
                std::cout << "password: " << user_info << std::endl;
            }
            else if(line_number % 8 == 2) 
            {
                std::cout << "id: " << user_info << std::endl;
            }
            else if(line_number % 8 == 3) 
            {
                std::cout << "profession: " << user_info << std::endl;
            }
            else if(line_number % 8 == 4) 
            {
                std::cout << "age: " << user_info << std::endl;
            }
            else if(line_number % 8 == 5) 
            {
                std::cout << "balance: " << user_info << std::endl;
            }
            else if (line_number % 8 == 6) 
            {
                std::cout << "debt: " << user_info << std::endl << std::endl;
                user_number++;
            }
            line_number++;
        }
        user_document.close();
    }
}

void login() 
{
    std::string name, password;
    login_successful = false;

    clearScreen();
    std::cout << "enter ur name: ";
    std::cin >> name;
    
    for (int i = 0; i < users.size(); ++i) 
    {
        if (name == users[i]->get_name()) 
        {
            std::cout << "pls enter ur password: ";
            std::cin >> password;

            while (password != users[i]->get_password()) 
            {
                std::cout << "ur password is wrong pls try again: ";
                std::cin >> password;
            }
            clearScreen();
            login_successful = true;
            current_user_index = i;
        }
    }
    if(login_successful){std::cout << "login successful." << std::endl;}
    else{std::cout << "user not found" << std::endl;}
}

void send_money() 
{
    std::string name;
    int id;
    float money;
    bool user_found = 0;
    std::cout << "who do u wanna send the money to? pls enter name and id." << std::endl;
    std::cout << "name: "; std::cin >> name; std::cout << "id: "; std::cin >> id;

    for (int i = 0; i < users.size(); ++i) 
    {
        if (name == users[i]->get_name() && id == users[i]->get_id()) 
        {
            std::cout << "how much do u send? "; std::cin >> money;
            clearScreen();
            users[i]->set_balance(users[i]->get_balance() + money);
            std::cout << money << " sent to " << users[i]->get_name() << std::endl;
            user_found = 1;
        }
    }
    clearScreen();
    if(!user_found) {std::cout << "user not found." << std::endl;}
}

void add_money() 
{
    float money;
    std::cout << "how much do you want to add: "; std::cin >> money;
    users[current_user_index]->set_balance(users[current_user_index]->get_balance() + money);
    clearScreen(); std::cout << users[current_user_index]->get_balance() << " added." << std::endl;
    
}

void account() 
{
    int choice;
    std::cout << "welcome " << users[current_user_index]->get_name() << std::endl;
    
    while (true) {
        users[current_user_index]->show_information();
        std::cout << "what do you want to do? (2) for add money, (1) for send money, (0) for quit: ";
        std::cin >> choice;

        if (choice == 1) {clearScreen(); send_money();}
        else if (choice == 0) {clearScreen(); break;}
        else if (choice == 2) {system("cls"); add_money();}
        else {std::cout << "ur input is invalid." << std::endl;}
    }
}

std::string client_instructions(zmq::message_t& request_, zmq::message_t& identity_, zmq::socket_t& socket_, std::string*& response_ptr, zmq::context_t& context)
{
    while (true) 
    {
        if(exit_check)
        {
            std::string exit_message = "Session terminated by the server";
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
                return received_message;
            }
            else 
            {
                return "";
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

void main_menu(std::string*& response_ptr, zmq::message_t& request, zmq::message_t& identity, zmq::socket_t& socket, zmq::context_t& context , std::string& choice)
{
    while (true) 
    {
        std::cout << "welcome to our system. what do you want to do? (1) for register, (2) for login, (0) for quit (3)for network testing, 4 for display users from document: ";
        //std::cin >> choice;
        std::getline(std::cin, choice);
        
        if (choice == "1") {create_new_user();}
        else if (choice == "0") {clearScreen(); break;}
        else if (choice == "2") {login(); if (login_successful) { account(); } }
        else if(choice == "3") 
        {
            std::thread user_inputThread(user_input, std::ref(response_ptr));
            std::thread messaging_funcThread(messaging_func, std::ref(request), std::ref(identity), std::ref(socket), std::ref(response_ptr), std::ref(context));
            user_inputThread.join();
            messaging_funcThread.join();  
        }
        else if (choice == "4") {display_users_from_document();}
        else {std::cout << "ur input is invalid." << std::endl;}
    }
}

void remote_main_menu(std::string*& response_ptr, zmq::message_t& request, zmq::message_t& identity, zmq::socket_t& socket, zmq::context_t& context , std::string& choice)
{
    while (true) 
    {
        if(choice != sentinel_code)
        {
            if(choice == "client is connected.")
            {
                std::cout << "READY" << std::endl;
            }
            else
            {
                std::cout << "welcome to our system. what do you want to do? (1) for register, (2) for login, (0) for quit (3)for network testing, 4 for display users from document: ";
                //std::cin >> choice;
                //std::getline(std::cin, choice);
                if (choice == "1") {create_new_user();}
                else if (choice == "0") {clearScreen(); break;}
                else if (choice == "2") {login(); if (login_successful) { account(); } }
                else if(choice == "3") 
                {
                    std::thread user_inputThread(user_input, std::ref(response_ptr));
                    std::thread messaging_funcThread(messaging_func, std::ref(request), std::ref(identity), std::ref(socket), std::ref(response_ptr), std::ref(context));
                    user_inputThread.join();
                    messaging_funcThread.join();  
                }
                else if (choice == "4") {display_users_from_document();}
                //else {std::cout << "ur input is invalid." << std::endl;}
            }
        }
    }
}

int main() 
{
    srand(time(NULL));
    txt_to_constructor();
    
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::router);
    socket.setsockopt(ZMQ_LINGER, 0);
    socket.bind("tcp://*:5555");
    
    zmq::message_t identity;
    zmq::message_t request;
    std::string response = sentinel_code;
    std::string* response_ptr = &response;
    std::string choice = client_instructions(request, identity, socket, response_ptr, context);

    remote_main_menu(response_ptr, request, identity, socket, context, choice);
    constructor_to_txt();
    for (int i = 0; i < users.size(); ++i) {delete users[i];}
    
    return 0;
}
