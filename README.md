# Simple Messaging System (C++ & ZeroMQ)

This project is a simple messaging system built using C++ and ZeroMQ.  
The main purpose is to improve my skills in multithreading, socket communication, and synchronization.

---

## Project Status

This project is still under development and may contain bugs or incomplete features.  
Features such as account management, authentication, and structured message handling are currently being developed (see `main.cpp`).  
Please note that this codebase is intended for educational and experimental purposes only.

---

## Planned Features

- A basic account system with name, password, ID, profession, age, balance, and debt
- User registration and login features
- Persistent data storage using file I/O
- In-app money transfer between users
- Client-server message communication via ZeroMQ sockets
- Text-based terminal interface
- Potential for future networked use beyond localhost

---

## Turkish Explanation

Bu proje, C++ ve ZeroMQ kullanılarak geliştirilmiş basit bir mesajlaşma sistemidir.  
Amaç, çoklu iş parçacığı (multithreading), soket haberleşmesi ve eşzamanlılık (senkronizasyon) konularında kendimi geliştirmektir.

Geliştirme hedefleri, kullanıcı kayıt sistemi, giriş işlemleri, para transferi ve mesajlaşma altyapısının oluşturulması yönündedir.

Proje henüz tamamlanmamıştır ve bazı bölümleri eksik veya hatalı olabilir.

---

## Build Instructions

### Requirements
- A C++17 compatible compiler
- ZeroMQ installed (`libzmq` and `cppzmq` headers)

### Compile

```bash
g++ -std=c++17 server.cpp -lzmq -o server
g++ -std=c++17 client.cpp -lzmq -o client
g++ -std=c++17 main.cpp -lzmq -o main
```

### Run

In separate terminals:

```bash
./server
```

```bash
./client
```

Or to run the standalone application:

```bash
./main
```

---

## License

This project is created for educational purposes and has no commercial intent.  
You are free to use, modify, and share it.
