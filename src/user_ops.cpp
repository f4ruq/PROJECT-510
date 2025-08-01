#include "user_ops.hpp"
#include "510.hpp"

std::vector<users*> user_data;

users::users(std::string user_name_, std::string password_){user_name = user_name_; password = password_;}

void users::set_user_name(std::string user_name){user_name = users::user_name;}

void users::set_password(std::string password){password = users::password;}

std::string users::get_password(){return password;}

std::string users::get_user_name(){return user_name;}

void create_new_user(std::string& user_name, std::string& password) 
{
    users* new_user = new users(user_name, password);
    user_data.push_back(new_user);
    std::cout << "User registered successfully." << std::endl;
}

void clean_memory()
{
    for (int i = 0; i < user_data.size(); ++i) {delete user_data[i];}
}

void login_(std::string& user_name, std::string& password) // the function named login_ because i defined "login" for window number
{
    login_successful = false;
    
    for (int i = 0; i < user_data.size(); ++i) 
    {
        if (user_name == user_data[i]->get_user_name()) 
        {

            while (password != user_data[i]->get_password()) 
            {
                std::cout << "ur password is wrong pls try again: ";
            }
            login_successful = true;
            current_user_index = i;
        }
    }
    if(login_successful){current_window = switch;}
    else{std::cout << "user not found" << std::endl;}
}
