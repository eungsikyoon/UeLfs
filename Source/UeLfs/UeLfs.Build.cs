// ----------------------------------------------------------------------------
// © 2022 Eungsik Yoon <yoon.eungsik@gmail.com>
// ----------------------------------------------------------------------------

using UnrealBuildTool;

public class UeLfs : ModuleRules
{
    public UeLfs(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.AddRange(
            new string[] {
				"Core",
				"CoreUObject",
				"InputCore",
				"Engine",
				"Slate",
				"SlateCore",
				"EditorStyle",
				"SourceControl",
				"Http",
				"Json",
				"JsonUtilities",
				"UnrealEd",
				"Projects",
				"PackagesDialog",
            }
        );
    }
}
