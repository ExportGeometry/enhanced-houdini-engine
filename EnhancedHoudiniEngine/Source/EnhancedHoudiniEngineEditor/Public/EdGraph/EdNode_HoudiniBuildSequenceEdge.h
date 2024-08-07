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
#include "SGraphNode.h"
#include "Templates/SharedPointer.h"

#include "EdNode_HoudiniBuildSequenceEdge.generated.h"

class UEdNode_HoudiniBuildSequenceEdge;
class UEdNode_HoudiniBuildSequenceNode;

class SEdNode_HoudiniBuildSequenceEdge : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SEdNode_HoudiniBuildSequenceEdge){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdNode_HoudiniBuildSequenceEdge* InNode);

	virtual bool RequiresSecondPassLayout() const override { return true; }
	virtual void PerformSecondPassLayout(const TMap< UObject*, TSharedRef<SNode> >& NodeToWidgetLookup) const override;
	virtual void UpdateGraphNode() override;

	// Calculate position for multiple nodes to be placed between a start and end point, by providing this nodes index and max expected nodes 
	void PositionBetweenTwoNodesWithOffset(const FGeometry& StartGeom, const FGeometry& EndGeom, int32 NodeIndex, int32 MaxNodes) const;

	// Edges don't need names. Can uncomment this if edge naming is needed later.
	/// void OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo);

protected:
	FSlateColor GetEdgeColor() const;

	const FSlateBrush* GetEdgeImage() const;

	EVisibility GetEdgeImageVisibility() const;
	EVisibility GetEdgeTitleVisbility() const;
};

UCLASS()
class ENHANCEDHOUDINIENGINEEDITOR_API UEdNode_HoudiniBuildSequenceEdge : public UEdGraphNode
{
	GENERATED_BODY()

public:
	//~UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	//~End UEdGraphNode interface
	
	void CreateConnections(UEdNode_HoudiniBuildSequenceNode* Start, UEdNode_HoudiniBuildSequenceNode* End);

	UEdNode_HoudiniBuildSequenceNode* GetStartNode();
	UEdNode_HoudiniBuildSequenceNode* GetEndNode();
};