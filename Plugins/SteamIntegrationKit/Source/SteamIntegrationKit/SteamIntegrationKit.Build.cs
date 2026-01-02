// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SteamIntegrationKit : ModuleRules
{
	public SteamIntegrationKit(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UMG",
				"Slate",
				"SlateCore",
				"Networking",
				"InputCore",
				"NetCore", 
				"OnlineSubsystem", 
				"OnlineSubsystemSteam",
				"OnlineSubsystemUtils"
			}
		);
	}
}
