// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ShooterSamProject : ModuleRules
{
	public ShooterSamProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ShooterSamProject",
			"ShooterSamProject/Variant_Platforming",
			"ShooterSamProject/Variant_Platforming/Animation",
			"ShooterSamProject/Variant_Combat",
			"ShooterSamProject/Variant_Combat/AI",
			"ShooterSamProject/Variant_Combat/Animation",
			"ShooterSamProject/Variant_Combat/Gameplay",
			"ShooterSamProject/Variant_Combat/Interfaces",
			"ShooterSamProject/Variant_Combat/UI",
			"ShooterSamProject/Variant_SideScrolling",
			"ShooterSamProject/Variant_SideScrolling/AI",
			"ShooterSamProject/Variant_SideScrolling/Gameplay",
			"ShooterSamProject/Variant_SideScrolling/Interfaces",
			"ShooterSamProject/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
