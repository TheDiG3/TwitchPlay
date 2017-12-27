# TwitchPlay
Unreal Engine 4 Plugin for Twitch Chat integration

Unreal's Marketplace Page: https://www.unrealengine.com/marketplace/twitchplay-plugin

Preview: https://www.youtube.com/watch?v=QCcoUbKkOjw

# Description

Two components available for use: TwitchIRCComponent and TwitchPlayComponent.

TwitchIRCComponent: enables communication back and forth with a Twitch channel chat. You can read each message (with relative username of the sender) by receiving a simple event (OnMessageReceived) or send your own messages to the chat (SendIRCMessage with SendTo enabled and chosen channel).

TwitchPlayComponent: in addition to what the TwitchIRCComponent does, this is a wonderful utility and time saver for when you want to implement user generated commands directly inside the game!

No need to do any parsing or checks on you side. Just Register a text command on the component and associate your own event that should fire whenever that user sends a chat message in the correct form (which is [DELIMITER]command[DELIMITER][OPTIONS]options[OPTIONS]. DELIMITER is '!' by default, but you can choose what you want. You can specify options for the command by using another delimiter, '#' by default, separated by ',').

You can also unregister commands that you don't need anymore at runtime. The only limitation is that a single object/function can be registered for a single command (if a second object tries to register it will overwrite the previous one's registration) at the moment. This might change in future API versions.

# Technical Details

The implementation uses FSockets and custom delegates to enable its functionalities.

Right now authentication still needs some work so that an actual error is received when the login info is wrong (I will need to authenticate the user asynchronously).

Only one object can subscribe to a custom command at a time. I might change that in later API versions.

Documentation: https://goo.gl/kjg3s0

Intended Platform: Windows (should work on other platforms too)
