// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class HoudiniOceanTest : ModuleRules
{
    //private string ModulePath
    //{
    //    get { return Path.GetDirectoryName(RuleCompiler.GetModuleFilename(this.GetType().Name)); }
    //}

    private string ThirdPartyPath
    {
        get { return "D:/Unreal Projects/HotTest/HoT/UE4/HoudiniOceanTest/3rdparty/"; }
    }

    public bool LoadLibs(TargetInfo Target)
    {
        bool isLibrarySupported = false;
        if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
        {
            isLibrarySupported = true;

            string PlatformString = (Target.Platform == UnrealTargetPlatform.Win64) ? "win64" : "win32";
            string LibrariesPath = ThirdPartyPath + PlatformString + "/";

            PublicAdditionalLibraries.Add(LibrariesPath + "libfftw3-3.lib");
            PublicAdditionalLibraries.Add(LibrariesPath + "libfftw3f-3.lib");
            PublicAdditionalLibraries.Add(LibrariesPath + "libfftw3l-3.lib");

            PublicIncludePaths.Add(LibrariesPath);
            PublicIncludePaths.Add(ThirdPartyPath + "include/");
        }

        //Definitions.Add(string.Format("WITH_FFTW_LIB_BINDING={0}", isLibrarySupported ? 1 : 0));

        return isLibrarySupported;
    }

	public HoudiniOceanTest(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "RHI", "RenderCore", "ShaderCore" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

        LoadLibs(Target);

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
