// Copyright (C) Simone Di Gravio <email: altairjp@gmail.com> - All Rights Reserved

#define DEBUG_MSG(msg) GEngine->AddOnScreenDebugMessage( -1 , 6 , FColor::Red , msg )

#include "Components/TwitchIRCComponent.h"
#include <string>

// Sets default values for this component's properties
UTwitchIRCComponent::UTwitchIRCComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTwitchIRCComponent::SetUserInfo(FString _oauth, FString _username, FString _channel)
{
	this->oauth_token_ = _oauth;
	this->username_ = _username;
	this->channel_ = _channel;
	this->b_has_run_user_setup_ = true;
}

bool UTwitchIRCComponent::SendIRCMessage(FString _message, bool _b_send_to, FString _channel)
{
	// Only operate on existing and connected sockets
	if (connection_socket_ != nullptr && connection_socket_->GetConnectionState() == ESocketConnectionState::SCS_Connected)
	{
		// If the user specified a receiver format the message appropriately ("PRIVMSG")
		if (_b_send_to)
		{
			_message = "PRIVMSG #" + _channel + " :" + _message;
		}
		_message += "\n"; // Using C style strlen needs a terminator character or it will crash
		TCHAR* serialized_message = _message.GetCharArray().GetData();
		int32 size = FCString::Strlen(serialized_message);
		int32 out_sent;
		return connection_socket_->Send((uint8*)TCHAR_TO_UTF8(serialized_message), size, out_sent);
	}
	else
	{
		return false;
	}
}

bool UTwitchIRCComponent::Connect(FString& _out_error)
{
	ISocketSubsystem* sss = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	TSharedRef<FInternetAddr> connection_addr = sss->CreateInternetAddr();
	ESocketErrors socket_error = sss->GetHostByName("irc.twitch.tv", *connection_addr); // Name resolution for Twitch IRC server

	if (socket_error != SE_NO_ERROR)
	{
		_out_error = "Could not resolve hostname!";
		return false; // if the host could not be resolved return false
	}

	// Set connection port
	const int32 port = 6667; // Standard IRC port
	connection_addr->SetPort(port);

	FSocket* ret_socket = sss->CreateSocket(NAME_Stream, TEXT("TwitchPlay Socket"), false);

	// Socket creation might fail on certain subsystems
	if (ret_socket == nullptr)
	{
		_out_error = "Could not create socket!";
		return false;
	}

	// Setting underlying connection parameters
	int32 out_size;
	ret_socket->SetReceiveBufferSize(2 * 1024 * 1024, out_size);
	ret_socket->SetReuseAddr(true);

	// Try connection
	bool b_has_connected = ret_socket->Connect(*connection_addr);

	// If we cannot connect destroy the socket and return
	if (!b_has_connected)
	{
		ret_socket->Close();
		sss->DestroySocket(ret_socket);

		_out_error = "Connection to Twitch IRC failed!";
		return false;
	}

	// If the connection was successful create a timer to start receiving data on the socket
	else
	{
		// TODO: Maybe move to thread?
		GetWorld()->GetTimerManager().SetTimer(this->receiving_timer_, this, &UTwitchIRCComponent::ReceiveData, 0.05f, true);
		this->connection_socket_ = ret_socket;
		return true;
	}
}

bool UTwitchIRCComponent::AuthenticateTwitchIRC(FString& _out_error)
{
	// If we don't have connection return an error
	if (connection_socket_ == nullptr)
	{
		_out_error = "Connection is not initialized. Call 'Connect' before authenticating";
		return false;
	}

	// No point in trying to connect if user info was not setup
	if (!b_has_run_user_setup_)
	{
		_out_error = "Can't authenticate. Setup user info first";
		return false;
	}

	bool pass_ok = SendIRCMessage("PASS " + this->oauth_token_, false);
	bool nick_ok = SendIRCMessage("NICK " + this->username_, false);

	// Since the channel join might not be executed if the channel was not setup join_ok default value must be true
	// for the method to return true if the two previous statements evaluate to true
	bool join_ok = true;
	if (this->channel_ != "")
	{
		join_ok = SendIRCMessage("JOIN #" + this->channel_, false);
	}

	bool b_success = pass_ok && nick_ok && join_ok;

	if (!b_success)
	{
		_out_error = "Failed to send authentication message";
		return false;
	}

	// TODO: Find a way to check for correct authentication
	// Twitch returns a welcome message ("Welcome, GLHF") or error (:tmi.twitch.tv * NOTICE :Error logging) upon successful login
	// Could use that in async to check for the connection
	return (b_success);
}

void UTwitchIRCComponent::ReceiveData()
{
	// If the socket does not exist just return
	if (connection_socket_ == nullptr)
	{
		return;
	}

	TArray<uint8> data;
	uint32 data_size;
	bool received = false;
	if (connection_socket_->HasPendingData(data_size))
	{
		received = true;
		data.SetNumUninitialized(data_size); // Make space for the data
		int32 data_read;
		connection_socket_->Recv(data.GetData(), data.Num(), data_read); // Receive the data. Hopefully the buffer is large enough
	}

	FString f_string_data = "";
	if (received)
	{
		const std::string c_string_data(reinterpret_cast<const char*>(data.GetData()), data.Num()); // Conversion from uint8 to char
		f_string_data = FString(c_string_data.c_str()); // Conversion from TCHAR to FString
	}

	if (f_string_data != "")
	{
		TArray<FString> usernames;
		TArray<FString> parsed_messages = ParseMessage(f_string_data, usernames);

		for (int32 cycle_index = 0; cycle_index < parsed_messages.Num(); cycle_index++)
		{
			OnMessageReceived.Broadcast(parsed_messages[cycle_index], usernames[cycle_index]); // Fires the message reception event
		}
	}
}

TArray<FString> UTwitchIRCComponent::ParseMessage(const FString _message, TArray<FString>& _out_sender_username, bool _b_filter_user_only)
{
	TArray<FString> ret_messages_content;

	TArray<FString> message_lines;
	_message.ParseIntoArrayLines(message_lines); // A single "message" from Twitch IRC could include multiple lines. Split them now

	// Parse each line into its parts
	// Each line from Twitch contains meta information and content
	// Also need to check if the message is a PING sent from Twitch to check if the connection is alive
	// This is in the form "PING :tmi.twitch.tv" to which we need to reply with "PONG :tmi.twitch.tv"
	for (int32 cycle_line = 0; cycle_line < message_lines.Num(); cycle_line++)
	{
		// If we receive a PING immediately reply with a PONG and skip the line parsing
		if (message_lines[cycle_line] == "PING :tmi.twitch.tv")
		{
			//DEBUG_MSG( "PING RECEIVED" );
			this->SendIRCMessage("PONG :tmi.twitch.tv", false);
			continue; // Skip line parsing
		}

		// Parsing line
		// Basic message form is ":twitch_username!twitch_username@twitch_username.tmi.twitch.tv PRIVMSG #channel :message here"
		// So we can split the message into two parts based off the ":" character: meta[0] and content[1..n]
		// Also have to account for	possible ":" inside the content itself
		TArray<FString> message_parts;
		message_lines[cycle_line].ParseIntoArray(message_parts, TEXT(":"));

		// Meta parsing
		// Meta info is split by whitespaces
		TArray<FString> meta;
		message_parts[0].ParseIntoArrayWS(meta);

		// Assume at this point the message is from a user, but just in case set it beforehand
		// This is so that we can return an "empty" user if the message was of any other kind
		// For example, messages from the server (like upon connection) don't have a username
		FString sender_username = "";
		if (meta[1] == "PRIVMSG") // Type of message should always be in position 1 (or at least I hope so)
		{
			// Username should be the first part before the first "!"
			meta[0].Split("!", &sender_username, nullptr);
		}

		// If user only message filtering is enabled and no username was found for this message line, skip it
		if (_b_filter_user_only && sender_username == "")
		{
			continue; // Skip line
		}

		// Some messages correspond to events sent by the server (JOIN etc.)
		// In that case the message part is only one
		if (message_parts.Num() > 1)
		{
			// Content of the message is composed by all parts of the message from message_parts[1] on
			FString message_content = message_parts[1];
			if (message_parts.Num() > 2)
			{
				for (int32 cycle_content = 2; cycle_content < message_parts.Num(); cycle_content++)
				{
					// Add back the ":" that was used as the splitter
					message_content += ":" + message_parts[cycle_content];
				}
			}
			ret_messages_content.Add(message_content);
			_out_sender_username.Add(sender_username);
		}
	}
	return ret_messages_content;
}

UTwitchIRCComponent::~UTwitchIRCComponent()
{
	if (connection_socket_ != nullptr)
	{
		connection_socket_->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(connection_socket_);
	}
}