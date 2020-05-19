#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <map>
#include <list>
#include <functional>

#include "Socket.h"
#include "pub-sub.cpp"
#include "split.h"

const int BUFSIZE {4096};

using namespace std;

class HandleConnection {
public:
	std::list<Subscriber> clients;
	fd_set readfds;
	Socket *masterSocket;
	std::vector<Publisher> rooms;
	std::map<int, std::map<string, long>> tokens;

	HandleConnection(Socket *master);
	//~HandleConnection();
	int selectClient();
	void addClient();
	void disconnect();
	void receiveMessage();
	void handleCommand(const string &command, const std::vector<string> &params, Subscriber &client);
	void addRoom(const string &name);
	bool isRoomCreated(const string &name);
	void subscribe(const std::vector<string> &params, Subscriber &client);
	void unsubscribe(const std::vector<string> &params, Subscriber &client);
	void sine();
	void silence();
	void echo(const std::vector<string> &params);
};