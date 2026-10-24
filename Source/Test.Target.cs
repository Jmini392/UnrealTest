using UnrealBuildTool;
using System.Collections.Generic;

public class TestTarget : TargetRules
{
	public TestTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Test");
	}
}
