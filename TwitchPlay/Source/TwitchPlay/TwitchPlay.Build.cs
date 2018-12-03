// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class TwitchPlay: ModuleRules
{
	public TwitchPlay(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));

		PublicDependencyModuleNames.AddRange(
			 new string[]
			 {
					 "Core",
					 "Sockets",
					 "Networking",
					 "CoreUObject",
					 "Engine",
			 }
			 );

		PrivateDependencyModuleNames.AddRange(
			 new string[]
			 {
				 // ... add private dependencies that you statically link with here ...	
			 }
			 );


		DynamicallyLoadedModuleNames.AddRange(
			 new string[]
			 {
				 // ... add any modules that your module loads dynamically here ...
			 }
			 );
	}
}
