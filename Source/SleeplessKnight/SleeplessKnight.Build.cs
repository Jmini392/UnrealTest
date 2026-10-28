using UnrealBuildTool;

public class SleeplessKnight : ModuleRules
{
	public SleeplessKnight(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Public/Private 폴더 없이 모듈 루트 기준으로 #include "Characters/SKCharacter.h" 형태를 쓴다.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"GameplayAbilities", "GameplayTags", "GameplayTasks",
			"UMG", "Slate", "SlateCore"
		});
	}
}
