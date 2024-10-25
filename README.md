Trying to create a websocket

- How do you choose the best port, can you let a user choose a port for the server ?

---

Goals:
- Make the server create a connection for someone to connect to or ping.
- Implement a client that can connect to the server. (like [this](https://beej.us/guide/bgnet/source/examples/showip.c) to test). 

---

### The Websocket Server

- The Websocket server maintains all the open connections with the clients (users).
- For an RTC (RealTime-Chat) the server would have to maintain the connection and 
broadcast the messages from one user to the other.


### The Websocket Client

- The users can connect to the websocket server and send and receive messages
