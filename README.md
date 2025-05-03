# Simple Messaging System (C++ & ZeroMQ)

This project is a simple messaging system built using C++ and ZeroMQ.  
The main purpose is to improve my skills in multithreading, socket communication, and synchronization.

---

## Bu ne işe yarar? (Türkçe Açıklama)

Bu proje, C++ ve ZeroMQ kullanılarak geliştirilmiş basit bir mesajlaşma sistemidir.  
Amaç, çoklu iş parçacığı, soket haberleşmesi ve senkronizasyon konularında kendimi geliştirmektir.

---

## Build Instructions

### Requirements
- A C++17 compatible compiler
- ZeroMQ installed (e.g., `libzmq` and `cppzmq` headers)

### Compile

```bash
g++ -std=c++17 server.cpp -lzmq -o server
g++ -std=c++17 client.cpp -lzmq -o client
```

### Run

In separate terminals:

```bash
./server
```

```bash
./client
```

---

## License

This project is created for educational purposes and has no commercial intent.  
You are free to use, modify, and share it.
