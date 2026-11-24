# POP3 Server

A basic, concurrent POP3 server written in C++.

## Features
- Admin CLI commands: `start [config-file]`, `stop`, `restart [config-file]`,
  `status`, `help`, and `quit`
- Concurrent client sessions using the supplied TCP socket library
- Configuration and account loading from files
- POP3 commands: `USER`, `PASS`, `STAT`, `LIST`, `RETR`, `DELE`, `RSET`,
  `NOOP`, and `QUIT`
- Mailboxes stored as numbered `.eml` files under the configured mailbox root

## Build
Use MSYS2/MinGW with g++:

```bash
g++ -std=c++11 -g -Wall -fexceptions -pthread -o bin/Debug/POP3SERVER.exe main.cpp cli.cpp mailbox.cpp pop3server.cpp pop3servercli.cpp pop3serverconfig.cpp pop3session.cpp serverconfig.cpp session.cpp socket.cpp tcpserver.cpp tcpserversocket.cpp tcpsocket.cpp -lws2_32
```

## Run
```bash
bin/Debug/POP3SERVER.exe
```

Run from the repository root to use `pop3.conf`. Configuration file paths for
`mailbox` and `accounts` are resolved relative to the config file. The account
file contains one `username password` pair per line; blank lines and lines
starting with `#` are ignored. Mail is stored in
`<mailbox>/<username>/<message-number>.eml`.
The included `alice@example.test` and `bob@example.test` accounts both use
`demo-password` for local testing only; replace these values before deployment.

The supplied default port is 110, which may require elevated privileges on
some systems. Use `start path-to-config` to select an alternate config file.

The server provides POP3 without TLS; use only on a trusted network with
non-production test credentials.
