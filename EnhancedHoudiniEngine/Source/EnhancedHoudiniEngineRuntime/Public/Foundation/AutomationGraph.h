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

#include "AutomationGraph.generated.h"

USTRUCT()
struct FAutomationGraphSupportedNodeInfo
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TSubclassOf<UAutomationGraphNode> NodeType;

	UPROPERTY()
	FText NewNodeMenuCategory;
};

UCLASS()
class ENHANCEDHOUDINIENGINERUNTIME_API UAutomationGraph : public UObject
{
	GENERATED_BODY()

public:
	virtual TArray<FAutomationGraphSupportedNodeInfo> GetSupportedNodeInfo() { return SupportedNodeInfo; }
	
	virtual void Reset();
	
	UPROPERTY()
	TArray<TObjectPtr<UAutomationGraphNode>> RootNodes;

	// In the editor, this object is responsible for configuring the node structure and updating RootNodes.
	UPROPERTY()
	TObjectPtr<UEdGraph> EditorGraph;
	
protected:
	UPROPERTY()
	TArray<FAutomationGraphSupportedNodeInfo> SupportedNodeInfo;
};