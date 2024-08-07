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
#include "HoudiniBuildSequenceGraph.h"

#include "HoudiniBuildManager.generated.h"

class UHoudiniBuildSequenceNode;
struct FHoudiniBuildSequenceInfo;
class AHoudiniAssetActor;
class UAutomationGraphNode;

// Can probably just use TPair<> instead, but I don't 100% trust the constructor for that is making a copy.
USTRUCT()
struct FEHEBuildSequenceDFSNode
{
	GENERATED_BODY()

public:
	FEHEBuildSequenceDFSNode() = default;
	FEHEBuildSequenceDFSNode(TObjectPtr<UAutomationGraphNode>& NewSqeuenceNode)
	{
		GraphNode = NewSqeuenceNode;
	}
	FEHEBuildSequenceDFSNode(TObjectPtr<UAutomationGraphNode>& NewSqeuenceNode, TSet<TObjectPtr<UAutomationGraphNode>>& NewAncestors)
	{
		GraphNode = NewSqeuenceNode;
		Ancestors = NewAncestors;
	}
	
	UPROPERTY()
	TObjectPtr<UAutomationGraphNode> GraphNode = nullptr;

	UPROPERTY()
	TSet<TObjectPtr<UAutomationGraphNode>> Ancestors;
};

UCLASS(Blueprintable)
class ENHANCEDHOUDINIENGINERUNTIME_API AHoudiniBuildManager : public AActor
{
	GENERATED_BODY()

// CONSTANTS
public:
	static constexpr double kBuildPollRateSec = 0.100;
	
public:
	AHoudiniBuildManager(const FObjectInitializer& Initializer);
	
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	void EditorTick(float DeltaSeconds);
	virtual bool ShouldTickIfViewportsOnly() const override { return true; } // enables editor tick

	void Run();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UHoudiniBuildSequenceGraph* SequenceGraph;
	
protected:
	void InitializeNodes();
	void RefreshBuildPreview();
	
	void PollActiveNodes();
	void ResetSequenceGraph();
	void Cancel();
	void PrintBuildOrder();

	// Uncomment for testing.
	// UPROPERTY()
	// TArray<TWeakObjectPtr<AHoudiniAssetActor>> PreviewActors;

	// Nodes that are actively being built. Order shouldn't matter, we are going to poll each of these on tick.
	UPROPERTY()
	TSet<TObjectPtr<UAutomationGraphNode>> ActiveNodes;

	double LastTimePolled = 0.0;
	bool bNeedsInitializeGraph = false;
};
