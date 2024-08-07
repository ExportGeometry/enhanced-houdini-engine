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

#include "Foundation/HoudiniBuildManager.h"

#include "EHERuntimeLoggingDefs.h"
#include "EngineUtils.h"
#include "HoudiniAssetActor.h"
#include "AutomationNodes/ClearLandscapeLayersNode.h"
#include "AutomationNodes/ConsoleCommandNode.h"
#include "Foundation/HoudiniBuildSequenceNode.h"
#include "HoudiniEngineRuntime/Private/HoudiniAssetComponent.h"

AHoudiniBuildManager::AHoudiniBuildManager(const FObjectInitializer& Initializer): Super(Initializer)
{
	bNeedsInitializeGraph = true;
	PrimaryActorTick.bCanEverTick = true;
	bIsSpatiallyLoaded = false; // don't unload this actor.
}

void AHoudiniBuildManager::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// no need to do anything if this is a CDO, template object, or editor preview object.
	if (bNeedsInitializeGraph && !HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject | RF_Transient))
	{
		InitializeNodes();
		bNeedsInitializeGraph = false;
	}
}

void AHoudiniBuildManager::Tick(float DeltaSeconds)
{
	// The BuildManager should only ever tick in the editor.
	// Super::Tick(DeltaSeconds);

#if WITH_EDITOR
	EditorTick(DeltaSeconds);
#endif
}

void AHoudiniBuildManager::EditorTick(float DeltaSeconds)
{
	PollActiveNodes();
}

void AHoudiniBuildManager::Run()
{
	if (!ActiveNodes.IsEmpty())
	{
		UE_LOG(LogEHERuntime, Warning, TEXT("error: AHoudiniBuildManager::Build() tried to build but there are already nodes actively building."));
		return;
	}

	// Refresh the build order to make sure we have the most up to date list of actors.
	InitializeNodes();
	ActiveNodes.Append(SequenceGraph->RootNodes);
}

void AHoudiniBuildManager::InitializeNodes()
{
	UWorld* CurrentWorld = GetWorld();
	if (!CurrentWorld || CurrentWorld->WorldType != EWorldType::Editor)
	{
		// If we aren't in the editor world, no point in doing anything.
		return;
	}

	if (!SequenceGraph)
	{
		UE_LOG(LogEHERuntime, Error, TEXT("error: AHoudiniBuildManager::InitializeNodes(): Expected a valid sequence graph."));
		return;
	}

	if (SequenceGraph->RootNodes.IsEmpty())
	{
		// Nothing to do...
		return;
	}

	ResetSequenceGraph();
	
	TMap<UHoudiniAsset*, TSet<AHoudiniAssetActor*>> ActorsByAsset;
	TMap<FName, TSet<AHoudiniAssetActor*>> ActorsByTag;
	for (TActorIterator<AActor> ActorItr(CurrentWorld); ActorItr; ++ActorItr)
	{
		AActor* CurrentActor = *ActorItr;
		auto* CurrentAssetActor = Cast<AHoudiniAssetActor>(CurrentActor);

		if (!CurrentAssetActor)
		{
			continue;
		}
		
		for (FName ActorTag : CurrentActor->Tags)
		{
			TSet<AHoudiniAssetActor*>& ActorSet = ActorsByTag.FindOrAdd(ActorTag);
			ActorSet.Add(CurrentAssetActor);
		}

		UHoudiniAssetComponent* CurrentAssetComponenet = CurrentAssetActor->GetHoudiniAssetComponent();
		if (CurrentAssetComponenet)
		{
			UHoudiniAsset* CurrentAsset = CurrentAssetComponenet->GetHoudiniAsset();
			if (CurrentAsset)
			{
				TSet<AHoudiniAssetActor*>& ActorSet = ActorsByAsset.FindOrAdd(CurrentAsset);
				ActorSet.Add(CurrentAssetActor);
			}
		}
	}
	
	TArray<FEHEBuildSequenceDFSNode> NodeStack;
	TSet<TObjectPtr<UAutomationGraphNode>> Visited;
	TSet<AHoudiniAssetActor*> AddedActors;
	
	for (TObjectPtr<UAutomationGraphNode> SequenceNode : SequenceGraph->RootNodes)
	{
		NodeStack.Add(FEHEBuildSequenceDFSNode(SequenceNode));
	}
	while (!NodeStack.IsEmpty())
	{
		FEHEBuildSequenceDFSNode DFSNode = NodeStack.Pop();
		TObjectPtr<UAutomationGraphNode> GraphNode = DFSNode.GraphNode;
		TSet<TObjectPtr<UAutomationGraphNode>> Ancestors = DFSNode.Ancestors; // copy of this node's ancestors

		if (Visited.Contains(GraphNode))
		{
			continue;
		}
		
		Visited.Add(GraphNode);
		Ancestors.Add(GraphNode);
		
		// Initialize Nodes Here ---------------------------------------------------------------------------------------
		if (auto* BuildSequenceNode = Cast<UHoudiniBuildSequenceNode>(GraphNode))
		{
			bool NodeInitialized = true;
			
			for (FName ActorTag : BuildSequenceNode->BuildInfo.ActorTags)
			{
				if (!ActorsByTag.Contains(ActorTag))
				{
					continue;
				}

				for (AHoudiniAssetActor* AssetActor : *ActorsByTag.Find(ActorTag))
				{
					if (!AddedActors.Contains(AssetActor))
					{
						NodeInitialized &= BuildSequenceNode->Add(AssetActor);
						AddedActors.Add(AssetActor);
					}
				}
			}
			for (UHoudiniAsset* AssetType : BuildSequenceNode->BuildInfo.AssetTypes)
			{
				if (!ActorsByAsset.Contains(AssetType))
				{
					continue;
				}

				for (AHoudiniAssetActor* AssetActor : *ActorsByAsset.Find(AssetType))
				{
					if (!AddedActors.Contains(AssetActor))
					{
						NodeInitialized &= BuildSequenceNode->Add(AssetActor);
						AddedActors.Add(AssetActor);
					}
				}
			}
			
			if (NodeInitialized)
			{
				BuildSequenceNode->Ready();
				// TODO(): Consider adding a warning if the node was initialized with no work items.
			}
			else
			{
				UE_LOG(LogEHERuntime, Error, TEXT("error: AHoudiniBuildManager::InitializeNodes(): Failed to initialize HoudiniBuildSequenceNode."));
			}
		}
		else if (auto* ClearLandscapeLayersNode = Cast<UAGN_ClearLandscapeLayers>(GraphNode))
		{
			ClearLandscapeLayersNode->Initialize(CurrentWorld);
		}
		else if (auto* ConsoleCommandNode = Cast<UAGN_ConsoleCommandBase>(GraphNode))
		{
			ConsoleCommandNode->Initialize(CurrentWorld);
		}
		else
		{
			GraphNode->Ready();
		}
		// End Node Initialization -------------------------------------------------------------------------------------
		
		for (TObjectPtr<UAutomationGraphNode> ChildNode : GraphNode->ChildNodes)
		{
			if (Ancestors.Contains(ChildNode))
			{
				UE_LOG(LogEHERuntime, Error, TEXT("Failed to construct final build order: A cycle exists in the build graph."))
				ResetSequenceGraph();
				return;
			}
			NodeStack.Add(FEHEBuildSequenceDFSNode(ChildNode, Ancestors));
		}
	}

	// Might remove this later, but keeping for now- for debug purposes
	RefreshBuildPreview();
	PrintBuildOrder();
}

void AHoudiniBuildManager::RefreshBuildPreview()
{
	TArray<TObjectPtr<UAutomationGraphNode>> NodeQueue;
	TSet<TObjectPtr<UAutomationGraphNode>> Visited;

	NodeQueue.Append(SequenceGraph->RootNodes);
	Visited.Append(SequenceGraph->RootNodes);

	while (!NodeQueue.IsEmpty())
	{
		// Unfortunately UE doesn't have a good queue implementation, so for now just use the inefficient RemoveAt(0)
		TObjectPtr<UAutomationGraphNode> GraphNode = NodeQueue[0];
		NodeQueue.RemoveAt(0);

		// Uncomment for testing
		/*
		if (auto* SequenceNode = Cast<UHoudiniBuildSequenceNode>(GraphNode))
		{
			for (TWeakObjectPtr<AHoudiniAssetActor> AssetActor : SequenceNode->GetHoudiniActors())
			{
				if (AssetActor.IsValid())
				{
					PreviewActors.Add(AssetActor);
				}
			}
		}*/

		for (TObjectPtr<UAutomationGraphNode> ChildNode : GraphNode->ChildNodes)
		{
			if (!Visited.Contains(ChildNode))
			{
				NodeQueue.Add(ChildNode);
				Visited.Add(ChildNode);
			}
		}
	}
}

void AHoudiniBuildManager::PollActiveNodes()
{
	double TimeDelta = FPlatformTime::Seconds() - LastTimePolled;
	if (TimeDelta < kBuildPollRateSec)
	{
		return;
	}
	
	TSet<TObjectPtr<UAutomationGraphNode>> ToAdd;
	TSet<TObjectPtr<UAutomationGraphNode>> ToRemove;
	
	for(TObjectPtr<UAutomationGraphNode> CurrentNode : ActiveNodes)
	{
		if (!CurrentNode)
		{
			UE_LOG(LogEHERuntime, Error, TEXT("error: AHoudiniBuildManager::Tick() BuildNode is invalid."));
			ToRemove.Add(CurrentNode);
			continue;
		}

		EAutomationGraphNodeState NodeState = CurrentNode->GetState();
		
		switch (NodeState)
		{
		case EAutomationGraphNodeState::Active:
			continue;
		case EAutomationGraphNodeState::Standby:
			CurrentNode->Activate();
			continue;
		case EAutomationGraphNodeState::Finished:
			for (UAutomationGraphNode* ChildNode : CurrentNode->ChildNodes)
			{
				if (ChildNode->CanActivate())
				{
					if (ChildNode->Activate())
					{
						ToAdd.Add(ChildNode);
					}
				}
			}
			
			ToRemove.Add(CurrentNode);
			continue;
		case EAutomationGraphNodeState::Expired:
		case EAutomationGraphNodeState::Error:
			ToRemove.Add(CurrentNode);
			continue;
		default:
			UE_LOG(LogEHERuntime, Error, TEXT("error: AHoudiniBuildManager::Tick() unexpected build state: %s."), *UEnum::GetValueAsString(NodeState));
			ToRemove.Add(CurrentNode);
			continue;
		}
	}

	ActiveNodes.Append(ToAdd);
	for(TObjectPtr<UAutomationGraphNode> RemoveNode : ToRemove)
	{
		ActiveNodes.Remove(RemoveNode);
	}

	LastTimePolled = FPlatformTime::Seconds();
}

void AHoudiniBuildManager::ResetSequenceGraph()
{
	SequenceGraph->Reset();

	// Uncomment for testing.
	// PreviewActors.Empty();
}

void AHoudiniBuildManager::Cancel()
{
	ActiveNodes.Empty();
	LastTimePolled = 0.0;
}


void AHoudiniBuildManager::PrintBuildOrder()
{
	// Uncomment for testing.
	/*
	FString BuildOrderString = "HOUDINI BUILD ORDER------------------------------------------------------------------\n";

	for (TWeakObjectPtr<AHoudiniAssetActor> WeakActor : PreviewActors)
	{
		if (!WeakActor.IsValid())
		{
			UE_LOG(LogEHERuntime, Warning, TEXT("warning: AEnhancedHoudiniActorManager::PrintBuildOrder() tried to print invalid HoudiniAssetActor."));
			continue;
		}

		BuildOrderString.Append(WeakActor.Get()->HoudiniAssetComponent->GetDisplayName());
		BuildOrderString.Append("\n");
	}

	BuildOrderString.Append("-------------------------------------------------------------------------------------\n");

	UE_LOG(LogEHERuntime, Log, TEXT("%s"), *BuildOrderString);
	*/
}

// CONSOLE COMMANDS ----------------------------------------------------------------------------------------------------
FAutoConsoleCommandWithWorldAndArgs GHoudiniBuildManagerBuildAllCmd(
	TEXT("houdini.BuildManager.BuildAll"),
	TEXT("Runs the Build command on all HoudiniBuildManagers in the scene."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
		[](const TArray<FString>& Args, UWorld* World)
		{
			if (!World || World->WorldType != EWorldType::Editor)
			{
				// If we aren't in the editor world, no point in doing anything.
				return;
			}

			for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr)
			{
				AActor* CurrentActor = *ActorItr;
				auto* BuildManager = Cast<AHoudiniBuildManager>(CurrentActor);

				if (!BuildManager)
				{
					continue;
				}

				BuildManager->Run();
			}
		}
	)
);

// END CONSOLE COMMANDS ------------------------------------------------------------------------------------------------