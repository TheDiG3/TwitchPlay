// Copyright (C) Simone Di Gravio <email: altairjp@gmail.com> - All Rights Reserved

#pragma once

#include "Components/TwitchIRCComponent.h"
#include "TwitchPlayComponent.generated.h"

/**
 * Declaration of delegate type for commands received from chat.
 * Delegate signature should receive three parameters:
 * _command_name (const FString&) - Name of the command received.
 * _command_options (const TArray<FString>&) - Additional array of options for the command being invoked.
 * _sender_username (const FString&) - Username of who triggered the command.
 */
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FOnCommandReceived, const FString&, _command_name, const TArray<FString>&, _command_options, const FString&, _sender_username);


/**
 * Works the same as UTwitchIRCComponent, but enables to subscribe to events that are fired on specific chat commands.
 * You can still send and receive messages to/from channel chat.
 * Subscribe to OnMessageReceived to know when a message has harrived.
 * Subscribe to specific commands by registering with RegisterCommand() to receive events for that command.
 * Only one object/function per command can be subscribed. Might change in later versions of the API.
 * You can change the default characters for commands/options encapsulation via SetupEncasulationChars().
 * Remember to first Connect(), SetUserInfo() and then AuthenticateTwitchIRC() before trying to send messages.
 */
UCLASS(ClassGroup = (TwitchAPI), meta = (BlueprintSpawnableComponent))
class TWITCHPLAY_API UTwitchPlayComponent : public UTwitchIRCComponent
{
	GENERATED_BODY()

public:

	// Character to use for command encapsulation. Commands will be read in the form CHAR_Command_CHAR (no spaces or underscores!)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Commands Setup")
		FString command_encapsulation_char_ = "!";

	/**
	 * Character to use for command options encapsulation. Commands will be read in the form CHAR_Option1[,Option2,..]_CHAR (no spaces or underscores!)
	 * Multiple options can be specified and will be split into an FString array upon parsing
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Commands Setup")
		FString options_encapsulation_char_ = "#";

private:

	/**
	 * Map of the command events currently bound.
	 * Each time a new command event is subscribed to, a new map entry is added.
	 * For each command only one function will be bound.
	 * NOTE: Only methods marked as UFUNCTION can be bound dinamically!
	 *
	 * TODO: Unbind all events on component destruction? I don't know if it would generate memory leaks if not done
	 */
	TMap<FString, FOnCommandReceived> bound_events_;

public:

	/**
	 * Constructor subscribes to message reception.
	 */
	UTwitchPlayComponent();

	/**
	 * Setups the encapsulation characters to use for commands and options.
	 *
	 * @param _command_char - Character(s) to use to encapsulate commands.
	 * @param _options_char - Character(s) to use to encapsulate command options.
	 */
	UFUNCTION(BlueprintCallable, Category = "Commands Setup")
		void SetupEncapsulationChars(const FString _command_char, const FString _options_char);

	/**
	 * Registers a command to receive an event whenever that command is called via chat.
	 * Only one function associated with a single object can be registered per command (as a delegate pointer!).
	 * You actually need to create a delegate, bind it to a function and pass that in by reference.
	 * If you try to register another function or another object with the same command the new function of that object will replace the previous one.
	 * If you need to fire multiple events when a single command is received consider having just one event calling all the others.
	 *
	 * @param _command_name - The command to register (CASE SENSITIVE).
	 * @param _callback_function - The function to fire when the event rises.
	 * @param _out_result - Result of the operation.
	 *
	 * @return Whether the registration was successfully completed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Commands Setup")
		bool RegisterCommand(const FString _command_name, const FOnCommandReceived& _callback_function, FString& _out_result);

	/**
	* Unregisters a command to stop receiving events whenever that command is called via chat.
	* Keep in mind that since each command can only be bound to a single function (and single object) unregistering that command will remove any function from any object.
	*
	* @param _command_name - The command to unregister (CASE SENSITIVE).
	* @param _callback_function - The command to unregister.
	* @param _out_result - Result of the operation.
	*
	* @return Whether the unregistration was successfully completed.
	*/
	UFUNCTION(BlueprintCallable, Category = "Commands Setup")
		bool UnregisterCommand(const FString _command_name, FString& _out_result);

	virtual ~UTwitchPlayComponent();

private:

	/**
	 * Handler for when a message is received.
	 * Should call the parsing method to search for commands/options and fire the corresponding event.
	 *
	 * @param _message - The message that was received.
	 * @param _username - Username of who sent the chat message
	 *
	 * NOTE: Method must be marked as UFUNCTION in order to bind a dynamic delegate to it!
	 */
	UFUNCTION()
		void MessageReceivedHandler(const FString& _message, const FString& _username);

	/**
	 * Parses the message and returns any command associated with the message.
	 *
	 * @param _message - The message to parse.
	 *
	 * @return The command found, if any. Returns "" if no command was found.
	 */
	FString GetCommandString(const FString& _message) const;

	/**
	* Parses the message and returns any command options associated with the message.
	*
	* @param _message - The message to parse.
	*
	* @return The array of options found, if any. Returns an empty array if no command option was found.
	*/
	TArray<FString> GetCommandOptionsStrings(const FString& _message) const;

	/**
	 * Gets the string delimited by the chosen delimiter string.
	 *
	 * @param _in_string - The string to search in.
	 * @param _delimiter - The delimiter characters for the string.
	 *
	 * @return String delimited by the delimiters characters. Returns "" if no delimited string was found.
	 */
	FString GetDelimitedString(const FString& _in_string, const FString& _delimiter) const;
};
