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

#include "Foundation/HoudiniBuildSequenceNode.h"

#include "EHERuntimeLoggingDefs.h"
#include "HoudiniAssetActor.h"
#include "Foundation/HoudiniBuildManager.h"
#include "HoudiniEngineRuntime/Private/HoudiniAssetComponent.h"

bool UHoudiniBuildWorkItem::Initialize(UHoudiniBuildSequenceNode* NewOwner, AHoudiniAssetActor* AssetActor)
{
	if (!NewOwner || !AssetActor || !AssetActor->GetHoudiniAssetComponent())
	{
		BuildState = EEHEBuildState::Error;
		return false;
	}

	Owner = NewOwner;
	ToBuild = TWeakObjectPtr<AHoudiniAssetActor>(AssetActor);
	BuildState = EEHEBuildState::Standby;

	return true;
}

bool UHoudiniBuildWorkItem::Build()
{
	if (BuildState == EEHEBuildState::Uninitialized)
	{
		UE_LOG(LogEHERuntime, Error, TEXT("error: UHoudiniBuildWorkItem::Build() attempted to build uninitialized work item"));
		return false;
	}
	if (BuildState == EEHEBuildState::Building)
	{
		UE_LOG(LogEHERuntime, Warning, TEXT("error: UHoudiniBuildWorkItem::Build() attempted to build item already in progress"));
		return true;
	}

	if (!ToBuild.IsValid())
	{
		UE_LOG(LogEHERuntime, Error, TEXT("error: UHoudiniBuildWorkItem::Build() HoudiniAsset is invalid"));
		return false;
	}

	UHoudiniAssetComponent* AssetComponent = ToBuild.Get()->GetHoudiniAssetComponent();
	if (!AssetComponent)
	{
		UE_LOG(LogEHERuntime, Error, TEXT("error: UHoudiniBuildWorkItem::Build() HoudiniAssetComponent is invalid"));
		return false;
	}

	BuildStarted();

	auto& OnPostOutputProcessingDelegate = AssetComponent->GetOnPostOutputProcessingDelegate();
	if (!OnPostOutputProcessingDelegate.IsBoundToObject(this))
	{
		PostOutputProcessingDelegateHande = OnPostOutputProcessingDelegate.AddUObject(this, &ThisClass::OnHoudiniAssetPostProcess);
	}

	return BuildInternal(AssetComponent);
}

void UHoudiniBuildWorkItem::BeginDestroy()
{
	if (ToBuild.IsValid())
	{
		UHoudiniAssetComponent* AssetComponent = ToBuild.Get()->GetHoudiniAssetComponent();
		if (AssetComponent)
		{
			auto& OnPostOutputProcessingDelegate = AssetComponent->GetOnPostOutputProcessingDelegate();
			if (PostOutputProcessingDelegateHande.IsValid())
			{
				OnPostOutputProcessingDelegate.Remove(PostOutputProcessingDelegateHande);
				PostOutputProcessingDelegateHande.Reset();
			}
		}
	}
	
	Super::BeginDestroy();
}

EEHEBuildState UHoudiniBuildWorkItem::GetBuildState()
{
	if (!ToBuild.IsValid())
	{
		UE_LOG(LogEHERuntime, Error, TEXT("error: UHoudiniBuildWorkItem::GetBuildState() asset is invalid"));
		BuildState = EEHEBuildState::Error;
	}
	else if (BuildState == EEHEBuildState::Building)
	{
		double TimeDelta = FPlatformTime::Seconds() - TimeStarted;

		if (TimeDelta >= Owner->BuildInfo.BuildFailTimeoutSec)
		{
			// TODO(): Double check that GetFullName is descriptive enough for debugging.
			UE_LOG(LogEHERuntime, Error, TEXT("error: houdini asset %s has been building for %.2lf seconds"), *ToBuild.Get()->GetFullName(), TimeDelta);
			BuildState = EEHEBuildState::Expired;
		}
		else if (TimeDelta >= Owner->BuildInfo.BuildWarnTimeoutSec)
		{
			UE_LOG(LogEHERuntime, Warning, TEXT("warning: houdini asset %s has been building for %.2lf seconds"), *ToBuild.Get()->GetFullName(), TimeDelta);
		}
	}

	return BuildState;
}

void UHoudiniBuildWorkItem::BuildStarted()
{
	BuildState = EEHEBuildState::Building;
	TimeStarted = FPlatformTime::Seconds();
}

void UHoudiniBuildWorkItem::OnHoudiniAssetPostProcess(UHoudiniAssetComponent* AssetComponent, bool Succeeded)
{
	if (BuildState != EEHEBuildState::Building)
	{
		// not an error since some other system may have decided to build this asset.
		return;
	}
	
	if (Succeeded)
	{
		// TODO(): Vanilla HE does not expose any information about if this asset finished with error or not. Update this
		//         section if SideFx ever adds something similar to my custom AssetComponent->MostRecentCookState flag.
		BuildState = EEHEBuildState::Finished;
	}
	else
	{
		UE_LOG(LogEHERuntime, Error, TEXT("error: UHoudiniBuildWorkItem::OnHoudiniAssetPostProcess() houdini asset failed to build"));
		BuildState = EEHEBuildState::Error;
	}
}

UHoudiniBuildSequenceNode::UHoudiniBuildSequenceNode(const FObjectInitializer& Initializer): Super(Initializer)
{
	Title = FText::FromString("Build HDA");
	WorkItemClass = UHoudiniBuildWorkItem::StaticClass();
}


bool UHoudiniBuildSequenceNode::Add(AHoudiniAssetActor* AssetActor)
{
	if (GetState() != EAutomationGraphNodeState::Uninitialized && GetState() != EAutomationGraphNodeState::Standby)
	{
		return false;
	}
	
	auto* NewWorkItem = NewObject<UHoudiniBuildWorkItem>(this, WorkItemClass);
	if(!NewWorkItem->Initialize(this, AssetActor))
	{
		UE_LOG(LogEHERuntime, Error, TEXT("error: failed to initilize HoudiniBuildWorkItem"));
		return false;
	}

	WorkItems.Add(NewWorkItem);
	return true;
}

bool UHoudiniBuildSequenceNode::Activate()
{
	if (GetState() == EAutomationGraphNodeState::Uninitialized)
	{
		UE_LOG(LogEHERuntime, Error, TEXT("error: UHoudiniBuildSequenceNode::Activate() attempted to build uninitialized sequence node"));
		return false;
	}
	if (GetState() == EAutomationGraphNodeState::Active)
	{
		UE_LOG(LogEHERuntime, Warning, TEXT("error: UHoudiniBuildSequenceNode::Activate() attempted to build sequence node already in progress"));
		return false;
	}

	SetState(EAutomationGraphNodeState::Active);
	
	for (TObjectPtr<UHoudiniBuildWorkItem> WorkItem : WorkItems)
	{
		if (!WorkItem)
		{
			UE_LOG(LogEHERuntime, Error, TEXT("error: UHoudiniBuildSequenceNode::Activate() invalid work item"));
			return false;
		}

		if(!WorkItem->Build())
		{
			return false;
		}
	}

	return true;
}

void UHoudiniBuildSequenceNode::Reset()
{
	WorkItems.Empty();
	SetState(EAutomationGraphNodeState::Uninitialized); // require initialization every time we run.
}

EAutomationGraphNodeState UHoudiniBuildSequenceNode::GetState()
{
	bFinishedWithError = false;
	
	if (WorkItems.Num() == 0)
	{
		SetState(EAutomationGraphNodeState::Uninitialized);
		return Super::GetState();
	}
	
	if (Super::GetState() == EAutomationGraphNodeState::Active)
	{
		int NumFinished = 0;
		
		for (TObjectPtr<UHoudiniBuildWorkItem> WorkItem : WorkItems)
		{
			if (!WorkItem)
			{
				UE_LOG(LogEHERuntime, Error, TEXT("error: UHoudiniBuildSequenceNode::GetBuildState() invalid work item"));
				SetState(EAutomationGraphNodeState::Error);
				return Super::GetState();
			}
			
			EEHEBuildState WorkItemState = WorkItem->GetBuildState();
			switch (WorkItemState)
			{
			case EEHEBuildState::Building:
				break;
			case EEHEBuildState::Finished:
				NumFinished++;
				break;
			// TODO(): Add this back in if vanilla HE ever supports it.
			/*case EEHEBuildState::FinishedWithError:
				NumFinished++;
				bFinishedWithError = true;
				break;*/
			case EEHEBuildState::Expired:
				SetState(EAutomationGraphNodeState::Expired);
				return Super::GetState();
			case EEHEBuildState::Standby:
			case EEHEBuildState::Error:
			default:
				SetState(EAutomationGraphNodeState::Error);
				return Super::GetState();
			}
		}

		if (NumFinished == WorkItems.Num())
		{
			SetState(EAutomationGraphNodeState::Finished);
		}
	}
	
	return Super::GetState();
}

FString UHoudiniBuildSequenceNode::GetMessageText()
{
	if (GetState() == EAutomationGraphNodeState::Finished && bFinishedWithError)
	{
		double TotalTime = TimeFinished - TimeStarted;
		return FString::Printf(TEXT("Finished in %.2lf Seconds (with errors)"), TotalTime); 
	}

	return Super::GetMessageText();
}

TArray<TWeakObjectPtr<AHoudiniAssetActor>> UHoudiniBuildSequenceNode::GetHoudiniActors()
{
	TArray<TWeakObjectPtr<AHoudiniAssetActor>> ToReturn;

	for (TObjectPtr<UHoudiniBuildWorkItem> WorkItem : WorkItems)
	{
		if (!WorkItem)
		{
			continue;
		}
		ToReturn.Add(WorkItem->GetAssetActor());
	}

	return ToReturn;
}