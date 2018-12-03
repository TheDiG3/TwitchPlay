// Copyright (C) Simone Di Gravio <email: altairjp@gmail.com> - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine/World.h"
#include "Runtime/Engine/Public/TimerManager.h"
#include "Components/ActorComponent.h"
#include "Networking.h"
#include "TwitchIRCComponent.generated.h"

/**
 * Declaration of delegate type for messages received from chat.
 * Delegate signature should receive two parameters:
 * _message (const FString&) - Message received.
 * _username (const FString&) - Username of who sent the message.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMessageReceived, const FString&, _message, const FString&, _username);

/**
 * Makes communication with Twitch IRC possible through UE4 sockets.
 * You can send and receive messages to/from channel chat.
 * Subscribe to OnMessageReceived to know when a message has harrived.
 * Remember to first Connect(), SetUserInfo() and then AuthenticateTwitchIRC() before trying to send messages.
 */
UCLASS(ClassGroup = (TwitchAPI), meta = (BlueprintSpawnableComponent))
class TWITCHPLAY_API UTwitchIRCComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Event called each time a message is received
	UPROPERTY(BlueprintAssignable, Category = "Message Events")
		FMessageReceived OnMessageReceived;

	// Authentication token. Need to get it from official Twitch API
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
		FString oauth_token_;

	// Username. Must be in lowercaps
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
		FString username_;

	// Channel to join upon successful connection	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
		FString channel_;

private:
	FSocket* connection_socket_;

	// Timer that handles the method receiving data from the socket
	FTimerHandle receiving_timer_;

	// Since the user setup should be run at least once use this to check if SetUserInfo was called
	// Used before trying to authenticate 
	bool b_has_run_user_setup_ = false;


public:

	// Sets default values for this component's properties
	UTwitchIRCComponent();

	/** Sets up the user info for the connection.
	 *
	 * @param _oauth - Oauth token to use. Get one from official Twitch APIs.
	 * @param _username - Username to login with. All low caps.
	 * @param _channel - The channel to join upon connection.
	 */
	UFUNCTION(BlueprintCallable, Category = "Setup")
		void SetUserInfo(const FString _oauth, const  FString _username, const FString _channel);

	// Send a message on the connected socket
	//
	// @param _b_send_to - Whether this message should be sent to a specific channel/user
	// @param _channel - The channel (or user) to send this message to
	//
	// @return Whether the message was sent correctly
	UFUNCTION(BlueprintCallable, Category = "Messages")
		bool SendIRCMessage(FString _message, UPARAM(DisplayName = "Send to channel") bool _b_send_to = true, FString _channel = "");

	/**
	 * Creates a socket and tries to connect to Twitch IRC server.
	 * Does NOT authenticate the user.
	 * TODO: It internally creates a timer to handle receiving of messages. Might move to a threaded solution later or.
	 *
	 * @param _out_error - The type of error that prevented the authentication.
	 *
	 * @return Whether the connection was created.
	 */
	UFUNCTION(BlueprintCallable, Category = "Setup")
		bool Connect(FString& _out_error);

	/**
	 * Authenticates the connection to Twitch IRC servers.
	 * Also joins the channel if any was specified inside the component.
	 *
	 * @param _out_error - The type of error that prevented the authentication.
	 *
	 * @return Whether authentication succedeed or not.
	 */
	UFUNCTION(BlueprintCallable, Category = "Setup")
		bool AuthenticateTwitchIRC(FString& _out_error);

	/**
	 * Receives data from the socket.
	 * This is supposed to be a method called by a timer or a threaded method.
	 */
	void ReceiveData();

	/**
	 * Parses the message received from Twitch IRC chat in order to only get the content of the message.
	 * Since a single "message" could actually include multiple lines an array of strings is returned.
	 *
	 * @param _message - Message to parse
	 * @param _out_sender_username - The username(s) of the message sender(s). In sync with the return array.
	 * @param _b_filter_user_only - Filter only messages sent by the users? This avoids receiving server messages.
	 *
	 * @return Parsed messages.
	 */
	TArray<FString> ParseMessage(const FString _message, TArray<FString>& _out_sender_username, bool _b_filter_user_only = false);

	// Handles closing the connection and freeing up the socket resources
	virtual ~UTwitchIRCComponent();
};
