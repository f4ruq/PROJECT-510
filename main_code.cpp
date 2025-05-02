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

using namespace std;

class customer
{
protected:
    string name, profession, password, clientMessage;
    int id;
    float balance, debt;
    int age;

public:
    customer(string name_, string profession_, string password_, int id_, float balance_, float debt_, int age_)
        : name(name_), profession(profession_), password(password_), id(id_), balance(balance_), debt(debt_), age(age_) {}

    void set_name(string name_) { name = name_; }
    void set_profession(string profession_) { profession = profession_; }
    int get_id() { return id; }
    void set_balance(float balance_) { balance = balance_; }
    void set_debt(float debt_) { debt = debt_; }
    void set_age(int age_) { age = age_; }
    string get_name() { return name; }
    string get_profession() { return profession; }
    float get_balance() { return balance; }
    float get_debt() { return debt; }
    int get_age() { return age; }
    void set_password(string password_) { password = password_; }
    string get_password() { return password; }

    void show_information() {
        cout << "name: " << name << endl << "profession: " << profession << endl;
        cout << "id: " << id << endl << "age: " << age << endl << "balance: " << balance << endl << "debt: " << debt << endl;
    }
};

vector<customer*> users;
int choice;
int current_user_index = -1;
bool login_successful = false;
//string clientMessage;
string* clientMessage_ptr;

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
    string name, profession, password;
    int id, age;
    float balance, debt;
    id = rand() % 999 + 100;
    balance = 0;
    debt = 0;
    
    clearScreen();
    cout << "Please enter your name: "; cin >> name;
    cout << "Please enter your profession: "; cin >> profession;
    cout << "Please enter your password: "; cin >> password;
    cout << "Please enter your age: "; cin >> age;

    customer* new_user = new customer(name, profession, password, id, balance, debt, age);
    users.push_back(new_user);
    
    clearScreen();
    cout << "User registered successfully." << endl;
}

void txt_to_constructor() 
{
    string user_info, profession, name, password;
    int id, age; float balance, debt;
    int line_number = 0;
    int user_number = 1;
    
    ifstream user_document;
    user_document.open("user_info.txt");

    if (!user_document.is_open()) 
    {
        cout << "error: unable to open the file." << endl;
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
    ofstream user_document;
    int i = 0;
    user_document.open("user_info.txt");
    
    if(user_document.is_open()) {
        while(i < users.size()) {
            user_document << users[i]->get_name() << endl;
            user_document << users[i]->get_password() << endl;
            user_document << to_string(users[i]->get_id()) << endl;
            user_document << users[i]->get_profession() << endl;
            user_document << to_string(users[i]->get_age()) << endl;
            user_document << to_string(users[i]->get_balance()) << endl;
            user_document << to_string(users[i]->get_debt()) << endl << endl;
            i++;
        }
    }
    else {cout << "error: unable to open file" << endl;}
    user_document.close();
}

void display_users_from_document() 
{
    string user_info;
    int line_number = 0;
    int user_number = 1;
    
    ifstream user_document;
    user_document.open("user_info.txt");

    if (!user_document.is_open()) 
    {
        cout << "error: unable to open the file." << endl;
    }
    else 
    {
        while (getline(user_document, user_info)) 
        {
            if(line_number % 8 == 0) 
            {
                cout << "user (" << user_number << ")'s info: " << endl;
                cout << "name: " << user_info << endl;
            }
            else if(line_number % 8 == 1) 
            {
                cout << "password: " << user_info << endl;
            }
            else if(line_number % 8 == 2) 
            {
                cout << "id: " << user_info << endl;
            }
            else if(line_number % 8 == 3) 
            {
                cout << "profession: " << user_info << endl;
            }
            else if(line_number % 8 == 4) 
            {
                cout << "age: " << user_info << endl;
            }
            else if(line_number % 8 == 5) 
            {
                cout << "balance: " << user_info << endl;
            }
            else if (line_number % 8 == 6) 
            {
                cout << "debt: " << user_info << endl << endl;
                user_number++;
            }
            line_number++;
        }
        user_document.close();
    }
}

void login() 
{
    string name, password;
    login_successful = false;

    clearScreen();
    cout << "enter ur name: ";
    cin >> name;
    
    for (int i = 0; i < users.size(); ++i) 
    {
        if (name == users[i]->get_name()) 
        {
            cout << "pls enter ur password: ";
            cin >> password;

            while (password != users[i]->get_password()) 
            {
                cout << "ur password is wrong pls try again: ";
                cin >> password;
            }
            clearScreen();
            login_successful = true;
            current_user_index = i;
        }
    }
    if(login_successful){cout << "login successful." << endl;}
    else{cout << "user not found" << endl;}
}

void send_money() 
{
    string name;
    int id;
    float money;
    bool user_found = 0;
    cout << "who do u wanna send the money to? pls enter name and id." << endl;
    cout << "name: "; cin >> name; cout << "id: "; cin >> id;

    for (int i = 0; i < users.size(); ++i) 
    {
        if (name == users[i]->get_name() && id == users[i]->get_id()) 
        {
            cout << "how much do u send? "; cin >> money;
            clearScreen();
            users[i]->set_balance(users[i]->get_balance() + money);
            cout << money << " sent to " << users[i]->get_name() << endl;
            user_found = 1;
        }
    }
    clearScreen();
    if(!user_found) {cout << "user not found." << endl;}
}

void add_money() 
{
    float money;
    cout << "how much do you want to add: "; cin >> money;
    users[current_user_index]->set_balance(users[current_user_index]->get_balance() + money);
    clearScreen(); cout << users[current_user_index]->get_balance() << " added." << endl;
    
}

void account() 
{
    int choice;
    cout << "welcome " << users[current_user_index]->get_name() << endl;
    
    while (true) {
        users[current_user_index]->show_information();
        cout << "what do you want to do? (2) for add money, (1) for send money, (0) for quit: ";
        cin >> choice;

        if (choice == 1) {clearScreen(); send_money();}
        else if (choice == 0) {clearScreen(); break;}
        else if (choice == 2) {system("cls"); add_money();}
        else {cout << "ur input is invalid." << endl;}
    }
}

string server(string* ptr)
{
    zmq::context_t context(1); 
    zmq::message_t identity; //client kimligi
    zmq::socket_t socket(context, zmq::socket_type::router); 
    socket.bind("tcp://*:5555"); 


    while (true) 
    { 
        zmq::message_t identity; 
        
        zmq::recv_result_t id_result = socket.recv(identity, zmq::recv_flags::none);
        if (!id_result) 
        {
            std::cerr << "hata: istemci kimliği alinamadi" << std::endl;
            continue; 
        }

        zmq::message_t request;
        
        zmq::recv_result_t req_result = socket.recv(request, zmq::recv_flags::none);
        if (!req_result) 
        {
            std::cerr << "hata: istemciden mesaj alinamadi" << std::endl;
            continue; 
        }
        string clientString = request.to_string();
        ptr = &clientString;
        return *ptr;
    
        
        if (stoi(clientString) == 1) {create_new_user();}
        else if (stoi(clientString) == 0) {clearScreen(); break;}
        else if (stoi(clientString) == 2) {login(); if (login_successful) { account(); } }
        else if(stoi(clientString) == 3) {cout << "client pointer message: " << server(clientMessage_ptr) << endl;}
        else if (stoi(clientString) == 4) {display_users_from_document();}
        else {cout << "ur input is invalid." << endl;}
        
        //break;
        //////////////////////////////////////////////////////////////////////////
        
        std::string reply;
        std::cout << "mesaj gir: "; 
        std::cin >> reply;
        zmq::message_t response(reply.size());
        memcpy(response.data(), reply.data(), reply.size());
        
    
        socket.send(identity, zmq::send_flags::sndmore); 
        socket.send(response, zmq::send_flags::none);    
    ////////////////////////////////////////////////////////////////////////////////
    }
}


int main() 
{
    srand(time(NULL));
    txt_to_constructor();
    
    while (true) 
    {
        cout << "welcome to our system. what do you want to do? (1) for register, (2) for login, (0) for quit (3)for network testing, 4 for display users from document: ";
        cin >> choice;
        
        if (choice == 1) {create_new_user();}
        else if (choice == 0) {clearScreen(); break;}
        else if (choice == 2) {login(); if (login_successful) { account(); } }
        else if(choice == 3) {cout << "client pointer message: " << server(clientMessage_ptr) << endl;}
        else if (choice == 4) {display_users_from_document();}
        else {cout << "ur input is invalid." << endl;}
    }
    constructor_to_txt();
    for (int i = 0; i < users.size(); ++i) {delete users[i];}
    
    return 0;
}
