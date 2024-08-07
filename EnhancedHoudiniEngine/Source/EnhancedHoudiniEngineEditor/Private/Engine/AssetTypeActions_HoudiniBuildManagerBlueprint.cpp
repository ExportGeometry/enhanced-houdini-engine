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

#include "Engine/AssetTypeActions_HoudiniBuildManagerBlueprint.h"

#include "BlueprintEditorModule.h"
#include "EnhancedHoudiniEngineEditorModule.h"
#include "Editor/HoudiniBuildManagerBlueprint.h"
#include "Engine/HoudiniBuildManagerFactory.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FAssetTypeActions_HoudiniBuildManagerBlueprint::FAssetTypeActions_HoudiniBuildManagerBlueprint(EAssetTypeCategories::Type NewAssetCategory)
{
	AssetCategory = NewAssetCategory;
}

FText FAssetTypeActions_HoudiniBuildManagerBlueprint::GetName() const
{ 
	return LOCTEXT("AssetTypeActions_HoudiniBuildManagerBlueprint", "Houdini Build Manager"); 
}

UClass* FAssetTypeActions_HoudiniBuildManagerBlueprint::GetSupportedClass() const
{
	return UHoudiniBuildManagerBlueprint::StaticClass();
}

// This fn is an exact copy of the base class fn. Needed because ShouldUseDataOnlyEditor is not virtual.
void FAssetTypeActions_HoudiniBuildManagerBlueprint::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (UObject* Object : InObjects)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(Object))
		{
			bool bLetOpen = true;
			if (!Blueprint->SkeletonGeneratedClass || !Blueprint->GeneratedClass)
			{
				bLetOpen = EAppReturnType::Yes == FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("FailedToLoadBlueprintWithContinue", "Blueprint could not be loaded because it derives from an invalid class.  Check to make sure the parent class for this blueprint hasn't been removed! Do you want to continue (it can crash the editor)?"));
			}
			if (bLetOpen)
			{
				FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
				TSharedRef< IBlueprintEditor > NewKismetEditor = BlueprintEditorModule.CreateBlueprintEditor(Mode, EditWithinLevelEditor, Blueprint, ShouldUseDataOnlyEditor(Blueprint));
			}
		}
		else
		{
			FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("FailedToLoadBlueprint", "Blueprint could not be loaded because it derives from an invalid class.  Check to make sure the parent class for this blueprint hasn't been removed!"));
		}
	}
}

UFactory* FAssetTypeActions_HoudiniBuildManagerBlueprint::GetFactoryForBlueprintType(UBlueprint* InBlueprint) const
{
	auto* BlueprintFactory = NewObject<UHoudiniBuildManagerBlueprintFactory>();
	BlueprintFactory->ParentClass = TSubclassOf<AHoudiniBuildManager>(*InBlueprint->GeneratedClass);
	return BlueprintFactory;
}

bool FAssetTypeActions_HoudiniBuildManagerBlueprint::ShouldUseDataOnlyEditor( const UBlueprint* Blueprint ) const
{
	// Works similar to the normal blueprint behavior, except also allows "data only mode" to show up with the blueprint
	// is first created.
	return FBlueprintEditorUtils::IsDataOnlyBlueprint(Blueprint) 
		&& !FBlueprintEditorUtils::IsLevelScriptBlueprint(Blueprint) 
		&& !FBlueprintEditorUtils::IsInterfaceBlueprint(Blueprint)
		&& !Blueprint->bForceFullEditor;
		// && !Blueprint->bIsNewlyCreated;
}

#undef LOCTEXT_NAMESPACE