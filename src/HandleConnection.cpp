#include "HandleConnection.h"

const double samplingRate = 8000; // sample rate = 8kHz (PCM-16 format)

// Constructor
HandleConnection::HandleConnection(std::unique_ptr<Socket> master) : masterSocket(std::move(master)) {}

int HandleConnection::selectClient() {
	struct timeval tv;
	int secondes = 10; // set timeout 10 sec
	tv.tv_sec = secondes;
	tv.tv_usec = 0;
	int max_sd = masterSocket->sock;
	FD_ZERO(&readfds); // initiailize my current set.
	FD_SET(masterSocket->sock, &readfds); //add master socket to set
	for(const auto &client : clients) {
		int sd = client.getSocket(); //socket descriptor
		
		//if valid socket descriptor then add to read list  
		if(sd > 0)   
			FD_SET( sd , &readfds);   
			 
		//highest file descriptor number, need it for the select function  
		if(sd > max_sd)   
			max_sd = sd; 
	}
	
	//wait for an activity on one of the sockets , timeout is 10 sec
	int ret = select(max_sd + 1, &readfds, nullptr, nullptr, &tv);
	
	if (ret < 0)
	{   
		cerr << "Select Error" << endl;
	} 
	
	return ret; 	
}

// Todo: remove client from list when client disconnect.
void HandleConnection::disconnect() {
	auto it = clients.begin();
	while(it != clients.end()) {
		it = find_if(it, clients.end(), [=](Subscriber s) { return s.getSocket() == 0; } );
		// remove client
		// todo : unsubscribe this client from the subscriber rooms
		clients.erase(it);
		it++;
	}
}

void HandleConnection::addClient() {
	//If something happened on the master socket, then its an incoming connection  
	if (FD_ISSET(masterSocket->sock, &readfds))   
	{   
		Socket *newSocket = masterSocket->accept();
		// Adding client to Clients list.
		int socket = newSocket->sock;
		Subscriber subscriber(socket);
		clients.push_back(subscriber);
	}
}

void HandleConnection::subscribe(const std::vector<string> &params, Subscriber &client) {
	for(const auto & param : params) {
		if(!isRoomCreated(param)) {
			cout << "Room " << param << " is not created" << endl;
			addRoom(param);
		} else {
			cout<<"Room "<< param << " is created"<< endl;
		}
		
		auto it = find_if(rooms.begin(), rooms.end(), [=](Publisher p){ return p.Name() == param;} );
		if(it != rooms.end()) {
			auto room = it->Name();
			auto socket = client.getSocket();
			for(auto it1 = tokens[room].begin(); it1 != tokens[room].end(); ++it1) {
				if(it1->first == socket)
					return;
			}
			long token = PubSub::Subscribe(*it, client);
			cout << "New Subscriber with Socket ID = "<< client.getSocket() <<" in room " << room << endl;
			std::map<int, long> pubTokens;
			pubTokens.insert(make_pair(client.getSocket(), token));
			
			/*for(map<int,long>::const_iterator eptr=pubTokens.begin();eptr != pubTokens.end(); eptr++)
				cout << "pubTokens : " << eptr->first << "  " << eptr->second << endl; 
			*/
			
			if(tokens.find(room) != tokens.end()) {
				tokens[room][client.getSocket()] = token;
			}
			tokens.insert(make_pair(it->Name(), pubTokens));
			
			#if 0
			cout << "=============== tokens =============" << endl;
			for( map<string, map<int,long> >::const_iterator it = tokens.begin(); it != tokens.end(); it++) {
				cout << it->first << "  ";
				for( map<int,long>::const_iterator it1 = it->second.begin(); it1 != it->second.end(); it1++){
					cout << it1->first << " " << it1->second << endl;
				}
			}
			#endif
			
		}
	}
}

void HandleConnection::unsubscribe(const std::vector<string> &params, Subscriber &client) {
	for(const auto &param : params) {
		auto it = find_if(rooms.begin(), rooms.end(), [=](Publisher p){ return p.Name() == param;} );
		auto room = it->Name();
		auto socket = client.getSocket();
		if(it != rooms.end()) {
			for(auto it1 = tokens[room].begin(); it1 != tokens[room].end(); ++it1) {
				if(it1->first == socket) {
					PubSub::Unsubscribe(*it, it1->second);
					cout << "Unsubscribe Client with Socket ID = " << socket << " from room "<< room << endl;
					tokens[room].erase(it1);
				}
			}
		}
	}		
}

void HandleConnection::echo(const std::vector<string> &params, const Subscriber &client) {
	std::string buffer_recv = params[0];
	std::vector<uint16_t> audio_data;
	//cout << "buffer_recv : " << buffer_recv << endl;
	
	if(!buffer_recv.empty())
	{
		unsigned int i = 0;
		while( i < buffer_recv.size())
		{
			uint16_t v =  std::stoi(buffer_recv.substr(i, 4), nullptr, 16);
			audio_data.push_back(v);
			i += 4;
		}
	}
	
	char * eptr;
	double startTime = strtod(params[1].c_str(), &eptr)/1000; // time in s
	double endTime = strtod(params[2].c_str(), &eptr)/1000; // time in s
	
	//cout << "\naudio_data : ";
	//for(auto v : audio_data) cout << std::hex << setw(4) << setfill('0') << v;
	
	cout << endl;
	AudioChunk audioChunk;
	audioChunk.setSamplesNumber(audio_data.size()-1);
	audioChunk.setSamplesBuffer(audio_data.size()-1 ,audio_data);
	audioChunk.TruncateBuffer(startTime, endTime);
	cout << "audio data after TrucateBuffer: " << endl;
	audioChunk.printBuffer();
	
	auto it = find_if(rooms.begin(), rooms.end(), [=](Publisher p){ return p.Name() == params[3];} );
	auto socket = client.getSocket();
	if(it != rooms.end())
	{
		auto room = it->Name();
		for(auto it1 = tokens[room].begin(); it1 != tokens[room].end(); ++it1)
		{
			if(it1->first == socket)
			{
				cout << "Publish truncate of audio audio chunk"<< endl;
				it->Publish(audioChunk.getSamplesBuffer());
				cout << "Subscriber with Socket ID=" << socket << " has received truncate of audio chunk from " << room << endl;
			}
		}
	}
}

void HandleConnection::silence(const std::vector<string> &params, const Subscriber &client) {
	
	if(!params.empty()) { 
		char * eptr;
		double duration = strtod(params[0].c_str(), &eptr)/1000;
		SilentAudioChunk silent(samplingRate, duration);
		
		silent.genrate_samples();
		auto silence_buffer = silent.getSamplesBuffer();
			
		for (int i = 0; i < 7; i++){
			cout << std::hex << setw(4) << setfill('0') << silence_buffer[i];
		}
		cout << endl;
		
		for(unsigned int i = 1; i  < params.size(); ++i) {
			auto it = find_if(rooms.begin(), rooms.end(), [=](Publisher p){ return p.Name() == params[i];} );
			auto socket = client.getSocket();
			if(it != rooms.end()) {
				auto room = it->Name();
				for(auto it1 = tokens[room].begin(); it1 != tokens[room].end(); ++it1) {
					if(it1->first == socket) {
						cout << "Publish silent wave audio chunk"<< endl;
						it->Publish(silence_buffer);
						cout << "Subscriber with Socket ID=" << socket << " has received silent audio chunk from " << room << endl;
					}
				}
			}
		}
	}
}


void HandleConnection::sine(const std::vector<string> &params, const Subscriber &client) {
	if(params.size() > 3) { 
		char *eptr;
		double duration = strtod(params[0].c_str(), &eptr)/1000; // en seconds
		double frequency = strtod(params[1].c_str(), &eptr);
		double amplitude = strtod(params[2].c_str(), &eptr);
		double phase = M_PI/2;

		SineWaveAudioChunk sin(samplingRate, duration, amplitude, frequency, phase);
		
		sin.genrate_samples();
		auto sineBuffer = sin.getSamplesBuffer();
		sin.printBuffer();
		
		for(unsigned int i = 3; i  < params.size(); ++i) {
			auto it = find_if(rooms.begin(), rooms.end(), [=](Publisher p){ return p.Name() == params[i];} );
			auto socket = client.getSocket();
			if(it != rooms.end()) {
				auto room = it->Name();
				for(auto it1 = tokens[room].begin(); it1 != tokens[room].end(); ++it1) {
					if(it1->first == socket) {
						cout << "Publish sine wave audio chunk"<< endl;
						it->Publish(sineBuffer);
						cout << "Subscriber with Socket ID=" << socket << " has received sine audio chunk from " << room << endl;
					}
				}
			}	
		}
	}
}

void HandleConnection::handleCommand(const string &command, const std::vector<string> &params, Subscriber &client)
{
	using action_arguments = std::vector<std::string>;
	std::map<std::string, std::function<bool(action_arguments const&)> > commands {
		{"subscribe", [&](action_arguments const&)
			{
				subscribe(params, client);
				return true;
			}
		},
		{"unsubscribe", [&](action_arguments const&)
			{
				unsubscribe(params, client);
				return true;
			}
		},
		{"silence", [&](action_arguments const&)
			{
				silence(params, client);
				return true;
			}
		},		
		{"sine", [&](action_arguments const&)
			{
				sine(params, client);
				return true;
			}
		},
		{"echo", [&](action_arguments const&)
			{
				echo(params, client);
				return true;
			}
		}	
	};
	
	auto what = commands.find(command);
	if (what == commands.end()) {
		std::cout << "Command Not Found" << std::endl;
	} else {
		auto success = (what->second)(params);
		//std::cout << (success ? "success" : "failure") << std::endl;
	}
}

void HandleConnection::receiveMessage() {
	//some IO operation on some other socket 
	char buffer[BUFSIZE];  //data buffer 
	
	for (auto &client : clients)   
	{   
		int sd = client.getSocket();
			 
		if (FD_ISSET( sd , &readfds))   
		{   
			//Check if it was for closing, and also read the incoming message.
			memset(buffer, 0, BUFSIZE);
			int bytesIn = recv(sd, buffer, BUFSIZE, 0);
			if(bytesIn < 0) {
				cerr << "recv failed !" << endl;
			}
			else if (bytesIn == 0) // if recv returns zero, that means the connection has been closed
			{
				//removeClient
				//sd.close();
				//client.getSocket() = 0;
			}
				 
			else  //Handle the commands and echo back the message that came in.
			{
				std::vector<std::string> arguments;
				std::string delims = " ";	//delimater : " " and "\n".
				
				arguments = split(buffer, delims);
				std::string command;
				std::vector<std::string> params;
				
				if(!arguments.empty()) {
					command = arguments[0];
					for (unsigned int i = 1; i < arguments.size(); ++i) {
						params.push_back(arguments[i]);
					}
				}
				
				if(!command.empty())
				{
					handleCommand(command, params, client); // handle command.
				}
			}   
		}   
	} 	
	
}

void HandleConnection::addRoom(const string &name) {
	rooms.push_back(Publisher(name));
	cout << "Create room " << name << endl;
	#if 0
	cout << "=============rooms==============" <<endl;
	for(auto room : rooms)
		cout << "-----room : " << room.Name() << "-----" <<endl;
	#endif
}	

bool HandleConnection::isRoomCreated(const string &name) {
	auto it = find_if(rooms.begin(), rooms.end(), [=](Publisher p){ return p.Name() == name;});
	return (it != rooms.end());
}
