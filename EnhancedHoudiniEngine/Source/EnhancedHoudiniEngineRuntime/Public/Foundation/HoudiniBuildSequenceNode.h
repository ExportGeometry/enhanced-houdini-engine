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

#pragma once

#include "AutomationGraphNode.h"
#include "HoudiniAsset.h"

#include "HoudiniBuildSequenceNode.generated.h"

class UEdNode_HoudiniBuildSequenceNode;
class UHoudiniAssetComponent;
class UHoudiniBuildSequenceNode;
class AHoudiniAssetActor;

USTRUCT(BlueprintType)
struct FHoudiniBuildSequenceInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSet<UHoudiniAsset*> AssetTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSet<FName> ActorTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double BuildWarnTimeoutSec = 15.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double BuildFailTimeoutSec = 60.0;

	// TODO(): Add back in if vanilla HE ever supports it.
	// If true, an HDA cook state of "finished with errors" counts as a success for the purposes of this graph.
	/// UPROPERTY(EditAnywhere, BlueprintReadWrite)
	// bool bAllowFinishedWithErrors = false;
};

UENUM()
enum class EEHEBuildState: uint8
{
	Uninitialized,
	Standby,
	Building,
	Finished,
	// TODO(): Add back in if vanilla HE ever supports it.
	// FinishedWithError, 
	Expired,
	Error
};

// Made this a class instead of a struct so we can bind this directly to Houdini delegates.
UCLASS()
class ENHANCEDHOUDINIENGINERUNTIME_API UHoudiniBuildWorkItem : public UObject
{
	GENERATED_BODY()

public:
	virtual bool Initialize(UHoudiniBuildSequenceNode* NewOwner, AHoudiniAssetActor* AssetActor);
	virtual bool Build();

	virtual void BeginDestroy() override;
	
	EEHEBuildState GetBuildState();

	TWeakObjectPtr<AHoudiniAssetActor> GetAssetActor() { return ToBuild; }
	
protected:
	virtual void BuildStarted();
	virtual bool BuildInternal(UHoudiniAssetComponent* AssetComponent) { return false; }
	virtual void OnHoudiniAssetPostProcess(UHoudiniAssetComponent* AssetComponent, bool Succeeded);
	
	UPROPERTY()
	TObjectPtr<UHoudiniBuildSequenceNode> Owner = nullptr;
	
	UPROPERTY()
	TWeakObjectPtr<AHoudiniAssetActor> ToBuild = nullptr;
	
	double TimeStarted = 0.0;
	EEHEBuildState BuildState = EEHEBuildState::Uninitialized;

	
	FDelegateHandle PostOutputProcessingDelegateHande;
};

UCLASS()
class ENHANCEDHOUDINIENGINERUNTIME_API UHoudiniBuildSequenceNode : public UAutomationGraphNode
{
	GENERATED_BODY()

public:
	UHoudiniBuildSequenceNode(const FObjectInitializer& Initializer);
	
	bool Add(AHoudiniAssetActor* AssetActor);

	//~UAutomationGraphNode interface.
	virtual bool Activate() override;
	virtual void Reset() override;
	virtual EAutomationGraphNodeState GetState() override;
	virtual FString GetMessageText() override;
	//~End UAutomationGraphNode interface.
	
	TArray<TWeakObjectPtr<AHoudiniAssetActor>> GetHoudiniActors();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Build HDA")
	FHoudiniBuildSequenceInfo BuildInfo;
	
protected:
	UPROPERTY()
	TSubclassOf<UHoudiniBuildWorkItem> WorkItemClass;
	
	UPROPERTY()
	TArray<TObjectPtr<UHoudiniBuildWorkItem>> WorkItems;

	UPROPERTY()
	bool bFinishedWithError = false;
};