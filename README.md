# Terminal Based Messaging System (C++ & ZeroMQ)

This project is a simple messaging system built using C++ and ZeroMQ.  
The main purpose is to improve my skills in multithreading, socket communication, and synchronization.

---

## Project Status

This project is still under development and may contain bugs or incomplete features.  
Please note that this codebase is intended for educational and experimental purposes only.  
Client-server message communication via ZeroMQ sockets works fine. You can compile and run both.

This messaging system is configured to run over localhost by default (for now).  
To run the client and server across different machines or networks:

- Make sure to replace `localhost` with the appropriate IP address in both client and server code.  
- Ensure that firewalls or network configurations allow the chosen port to be accessible.  
- You’ll need to configure port forwarding or use a VPN/tunneling solution depending on your environment.

---

## Turkish Explanation

Bu proje, C++ ve ZeroMQ kullanılarak geliştirilmiş basit bir mesajlaşma sistemidir.  
Proje hâlâ geliştirme aşamasındadır ve bazı hatalar veya tamamlanmamış özellikler içerebilir.  
Lütfen bu kod tabanının yalnızca eğitim ve deneysel amaçlarla hazırlandığını unutmayın.

ZeroMQ soketleri üzerinden istemci-sunucu mesaj iletişimi düzgün şekilde çalışmaktadır.  
İstemci ve sunucu taraflarını derleyip çalıştırabilirsiniz.

Bu mesajlaşma sistemi varsayılan olarak `localhost` üzerinde çalışacak şekilde yapılandırılmıştır (şimdilik).  
İstemci ve sunucuyu farklı cihazlar veya ağlar arasında çalıştırmak için:

- Hem istemci hem de sunucu kodundaki `localhost` ifadesini uygun IP adresiyle değiştirin.  
- Seçilen portun erişilebilir olduğundan emin olmak için güvenlik duvarı ve ağ ayarlarını kontrol edin.  
- Ortamınıza bağlı olarak port yönlendirme, VPN veya tünelleme yöntemlerinden birini yapılandırmanız gerekebilir.

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

---

## License

This project is created for educational purposes and has no commercial intent.  
You are free to use, modify, and share it.
