// Copyright © Mason Stevenson
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted (subject to the limitations in the disclaimer
// below) provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
// THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
// NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "EnhancedHoudiniEngineEditorModule.h"

#include "AssetToolsModule.h"
#include "EHEEditorLoggingDefs.h"
#include "EdGraph/HoudiniBuildSequenceGraphNodeFactory.h"
#include "Editor/HoudiniBuildSequenceGraphEditorStyle.h"
#include "Engine/AssetTypeActions_HoudiniBuildManagerBlueprint.h"
#include "Logging/LogMacros.h"

#define LOCTEXT_NAMESPACE "FEnhancedHoudiniEngineModule"

void FEnhancedHoudiniEngineEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	UE_LOG(LogEHEEditor, Log, TEXT("Starting EnhancedHoudiniEngineEditorModule."));

	FHoudiniBuildSequenceGraphEditorStyle::Initialize();
	
	BSGNodeFactory = MakeShareable(new FHoudiniBuildSequenceGraphNodeFactory());
	FEdGraphUtilities::RegisterVisualNodeFactory(BSGNodeFactory);
	
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	BuildManagerAssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory(
		FName(TEXT("Houdini Engine Custom")), // pretty sure this needs to match the display name exactly, because of the new AssetDefinition system
		NSLOCTEXT("AssetTypeActions", "HoudiniEngineCustomCategory", "Houdini Engine Custom")
	);
	TSharedRef<IAssetTypeActions> BuildManagerBlueprintAction = MakeShareable(new FAssetTypeActions_HoudiniBuildManagerBlueprint(BuildManagerAssetCategoryBit));
	RegisterAssetTypeAction(AssetTools, BuildManagerBlueprintAction);
}

void FEnhancedHoudiniEngineEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	UE_LOG(LogEHEEditor, Log, TEXT("Shutting down EnhancedHoudiniEngineEditorModule."));
	
	if (BSGNodeFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(BSGNodeFactory);
		BSGNodeFactory.Reset();
	}
	
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (auto& AssetTypeAction : CreatedAssetTypeActions)
		{
			if (AssetTypeAction.IsValid())
			{
				AssetToolsModule.UnregisterAssetTypeActions(AssetTypeAction.ToSharedRef());
			}
		}
	}
	CreatedAssetTypeActions.Empty();

	FHoudiniBuildSequenceGraphEditorStyle::Shutdown();
}

void FEnhancedHoudiniEngineEditorModule::RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
{
	AssetTools.RegisterAssetTypeActions(Action);
	CreatedAssetTypeActions.Add(Action);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FEnhancedHoudiniEngineEditorModule, EnhancedHoudiniEngineEditorModule);