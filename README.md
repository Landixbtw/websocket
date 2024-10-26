Trying to create a websocket

- You can now connect to the server from any machine in your local network
- The server send a nice message to the client when connecting. 

- How can we keep the client "online", right now it closes after receiving the message.

### The Websocket Server

- The Websocket server maintains all the open connections with the clients (users).
- For an RTC (RealTime-Chat) the server would have to maintain the connection and 
broadcast the messages from one user to the other.


### The Websocket Client

- The users can connect to the websocket server and send and receive messages
