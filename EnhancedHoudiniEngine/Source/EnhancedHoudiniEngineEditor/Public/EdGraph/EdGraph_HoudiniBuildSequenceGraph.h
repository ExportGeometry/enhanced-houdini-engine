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
#include "ConnectionDrawingPolicy.h"
#include "Foundation/AutomationGraphNode.h"
#include "Foundation/HoudiniBuildSequenceGraph.h"

#include "EdGraph_HoudiniBuildSequenceGraph.generated.h"

class UEdNode_HoudiniBuildSequenceNode;

/** Action to add a node to the graph */
USTRUCT()
struct FAssetSchemaAction_HBSG_NewNode : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

public:
	FAssetSchemaAction_HBSG_NewNode(): NodeClass(nullptr) {}

	FAssetSchemaAction_HBSG_NewNode(const FText& InNodeCategory, const FText& InMenuDesc, const FText& InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping), NodeClass(nullptr) {}

	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;

	UPROPERTY()
	TSubclassOf<UAutomationGraphNode> NodeClass;
};

/** Action to add a node to the graph */
USTRUCT()
struct FAssetSchemaAction_HBSG_NewEdge : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

public:
	FAssetSchemaAction_HBSG_NewEdge() = default;

	FAssetSchemaAction_HBSG_NewEdge(const FText& InNodeCategory, const FText& InMenuDesc, const FText& InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping) {}

	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
};

class FConnectionDrawingPolicy_HoudiniBuildSequenceGraph : public FConnectionDrawingPolicy
{
protected:
	UEdGraph* GraphObj;
	TMap<UEdGraphNode*, int32> NodeWidgetMap;

public:
	FConnectionDrawingPolicy_HoudiniBuildSequenceGraph(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj);

	// FConnectionDrawingPolicy interface 
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params) override;
	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries, FArrangedChildren& ArrangedNodes) override;
	virtual void DrawSplineWithArrow(const FGeometry& StartGeom, const FGeometry& EndGeom, const FConnectionParams& Params) override;
	virtual void DrawSplineWithArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FConnectionParams& Params) override;
	virtual void DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin) override;
	virtual FVector2D ComputeSplineTangent(const FVector2D& Start, const FVector2D& End) const override;
	virtual void DetermineLinkGeometry(FArrangedChildren& ArrangedNodes, TSharedRef<SWidget>& OutputPinWidget, UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FArrangedWidget*& StartWidgetGeometry, FArrangedWidget*& EndWidgetGeometry) override;
	// End of FConnectionDrawingPolicy interface
};

UCLASS()
class ENHANCEDHOUDINIENGINEEDITOR_API UEdGraphSchema_HoudiniBuildSequenceGraph : public UEdGraphSchema
{
	GENERATED_BODY()

public:
	//~EdGraphSchema Interface
	virtual EGraphType GetGraphType(const UEdGraph* TestEdGraph) const override;
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual void GetContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const override;
	virtual bool TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const override;
	virtual bool CreateAutomaticConversionNodeAndConnections(UEdGraphPin* PinA, UEdGraphPin* PinB) const override;

	virtual class FConnectionDrawingPolicy* CreateConnectionDrawingPolicy(
		int32 InBackLayerID,
		int32 InFrontLayerID,
		float InZoomFactor,
		const FSlateRect& InClippingRect,
		class FSlateWindowElementList& InDrawElements,
		class UEdGraph* InGraphObj
	) const override;

	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
	virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const override;
	virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const override;
	virtual void BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const override;

	virtual UEdGraphPin* DropPinOnNode(
		UEdGraphNode* InTargetNode,
		const FName& InSourcePinName,
		const FEdGraphPinType& InSourcePinType,
		EEdGraphPinDirection InSourcePinDirection
	) const override;

	virtual bool SupportsDropPinOnNode(
		UEdGraphNode* InTargetNode,
		const FEdGraphPinType& InSourcePinType,
		EEdGraphPinDirection InSourcePinDirection,
		FText& OutErrorMessage
	) const override;

	/// TODO(): Figure out if these are needed.
	/// virtual bool IsCacheVisualizationOutOfDate(int32 InVisualizationCacheID) const override;
	/// virtual int32 GetCurrentVisualizationCacheID() const override;
	/// virtual void ForceVisualizationCacheClear() const override;
	//~End EdGraphSchema Interface

protected:
	void GetBreakLinkToSubMenuActions(class UToolMenu* Menu, class UEdGraphPin* SelectedGraphPin);
};

UCLASS(MinimalAPI)
class UEdGraph_HoudiniBuildSequenceGraph : public UEdGraph
{
	GENERATED_BODY()

public:
	UHoudiniBuildSequenceGraph* GetBuildSequenceGraph();
	void RebuildSequenceGraph();
};
