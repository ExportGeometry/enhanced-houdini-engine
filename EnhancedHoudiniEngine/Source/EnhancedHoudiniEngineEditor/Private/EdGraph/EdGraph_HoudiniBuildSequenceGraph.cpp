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

#include "EdGraph/EdGraph_HoudiniBuildSequenceGraph.h"

#include "EHEEditorLoggingDefs.h"
#include "GraphEditorActions.h"
#include "AutomationNodes/ClearLandscapeLayersNode.h"
#include "EdGraph/EdNode_HoudiniBuildSequenceEdge.h"
#include "EdGraph/EdNode_HoudiniBuildSequenceNode.h"
#include "Foundation/AutomationGraphNode.h"
#include "Framework/Commands/GenericCommands.h"

#define LOCTEXT_NAMESPACE "EdGraphSchema_HoudiniBuildSequenceGraph"

UEdGraphNode* FAssetSchemaAction_HBSG_NewNode::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	const FScopedTransaction Transaction(LOCTEXT("AssetSchemaAction_HBSG_NewNode", "Houdini Build Sequence Graph: New Node"));
	ParentGraph->Modify();
	if (FromPin != nullptr)
	{
		FromPin->Modify();	
	}
	
	// First construct the underlying GraphNode
	auto* ParentEditorBSG = CastChecked<UEdGraph_HoudiniBuildSequenceGraph>(ParentGraph);
	UHoudiniBuildSequenceGraph* ParentBSG = ParentEditorBSG->GetBuildSequenceGraph();
	auto* NewSequenceNode = NewObject<UAutomationGraphNode>(ParentBSG, NodeClass, NAME_None, RF_Transactional);
	
	if (auto* LayerClearNode = Cast<UAGN_ClearLandscapeLayers>(NewSequenceNode))
	{
		// Assume the user wants to write to an edit layer named "Procedural"
		LayerClearNode->EditLayers.Add("Procedural");
	}

	// Then construct the editor node
	FGraphNodeCreator<UEdNode_HoudiniBuildSequenceNode> NodeCreator(*ParentGraph);
	UEdNode_HoudiniBuildSequenceNode* NewGraphNode = NodeCreator.CreateUserInvokedNode(bSelectNewNode); // node must be UserInvoked in order to allow for renaming on create
	NewGraphNode->SequenceNode = NewSequenceNode;

	// This calls Node->CreateNewGuid(), Node->PostPlacedNewNode(), and Node->AllocateDefaultPins()
	NodeCreator.Finalize();
	NewGraphNode->AutowireNewNode(FromPin);
	
	NewGraphNode->NodePosX = Location.X;
	NewGraphNode->NodePosY = Location.Y;

	ParentEditorBSG->RebuildSequenceGraph();
	ParentBSG->PostEditChange();
	ParentBSG->MarkPackageDirty();
	
	return NewGraphNode;
}

UEdGraphNode* FAssetSchemaAction_HBSG_NewEdge::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	const FScopedTransaction Transaction(LOCTEXT("AssetSchemaAction_HBSG_NewEdge", "Houdini Build Sequence Graph: New Edge"));
	ParentGraph->Modify();
	if (FromPin != nullptr)
	{
		FromPin->Modify();	
	}

	auto* ParentEditorBSG = CastChecked<UEdGraph_HoudiniBuildSequenceGraph>(ParentGraph);
	UHoudiniBuildSequenceGraph* ParentBSG = ParentEditorBSG->GetBuildSequenceGraph();
	
	FGraphNodeCreator<UEdNode_HoudiniBuildSequenceEdge> NodeCreator(*ParentGraph);
	UEdNode_HoudiniBuildSequenceEdge* NewEdgeNode = NodeCreator.CreateNode();

	// This calls Node->CreateNewGuid(), Node->PostPlacedNewNode(), and Node->AllocateDefaultPins()
	NodeCreator.Finalize();
	NewEdgeNode->AutowireNewNode(FromPin);

	NewEdgeNode->NodePosX = Location.X;
	NewEdgeNode->NodePosY = Location.Y;

	ParentEditorBSG->RebuildSequenceGraph();
	ParentBSG->PostEditChange();
	ParentBSG->MarkPackageDirty();

	return NewEdgeNode;
}

FConnectionDrawingPolicy_HoudiniBuildSequenceGraph::FConnectionDrawingPolicy_HoudiniBuildSequenceGraph(
	int32 InBackLayerID,
	int32 InFrontLayerID,
	float ZoomFactor,
	const FSlateRect& InClippingRect,
	FSlateWindowElementList& InDrawElements,
	UEdGraph* InGraphObj) : FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements), GraphObj(InGraphObj)
{
}

void FConnectionDrawingPolicy_HoudiniBuildSequenceGraph::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params)
{
	Params.AssociatedPin1 = OutputPin;
	Params.AssociatedPin2 = InputPin;
	Params.WireThickness = 1.5f;

	const bool bDeemphasizeUnhoveredPins = HoveredPins.Num() > 0;
	if (bDeemphasizeUnhoveredPins)
	{
		ApplyHoverDeemphasis(OutputPin, InputPin, /*inout*/ Params.WireThickness, /*inout*/ Params.WireColor);
	}
}

void FConnectionDrawingPolicy_HoudiniBuildSequenceGraph::Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes)
{
	// Build an acceleration structure to quickly find geometry for the nodes
	NodeWidgetMap.Empty();
	for (int32 NodeIndex = 0; NodeIndex < ArrangedNodes.Num(); ++NodeIndex)
	{
		FArrangedWidget& CurWidget = ArrangedNodes[NodeIndex];
		TSharedRef<SGraphNode> ChildNode = StaticCastSharedRef<SGraphNode>(CurWidget.Widget);
		NodeWidgetMap.Add(ChildNode->GetNodeObj(), NodeIndex);
	}

	// Now draw
	FConnectionDrawingPolicy::Draw(InPinGeometries, ArrangedNodes);
}

void FConnectionDrawingPolicy_HoudiniBuildSequenceGraph::DrawSplineWithArrow(const FGeometry& StartGeom, const FGeometry& EndGeom, const FConnectionParams& Params)
{
	// Get a reasonable seed point (halfway between the boxes)
	const FVector2D StartCenter = FGeometryHelper::CenterOf(StartGeom);
	const FVector2D EndCenter = FGeometryHelper::CenterOf(EndGeom);
	const FVector2D SeedPoint = (StartCenter + EndCenter) * 0.5f;

	// Find the (approximate) closest points between the two boxes
	const FVector2D StartAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(StartGeom, SeedPoint);
	const FVector2D EndAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(EndGeom, SeedPoint);

	DrawSplineWithArrow(StartAnchorPoint, EndAnchorPoint, Params);
}

void FConnectionDrawingPolicy_HoudiniBuildSequenceGraph::DrawSplineWithArrow(const FVector2D& StartAnchorPoint, const FVector2D& EndAnchorPoint, const FConnectionParams& Params)
{
	// bUserFlag1 indicates that we need to reverse the direction of connection (used by debugger)
	const FVector2D& ArrowStartPoint = Params.bUserFlag1 ? EndAnchorPoint : StartAnchorPoint;
	const FVector2D& ArrowEndPoint = Params.bUserFlag1 ? StartAnchorPoint : EndAnchorPoint;

	// Should this be scaled by zoom factor?
	const float LineSeparationAmount = 4.5f;

	const FVector2D DeltaPos = ArrowEndPoint - ArrowStartPoint;
	const FVector2D UnitDelta = DeltaPos.GetSafeNormal();
	const FVector2D Normal = FVector2D(DeltaPos.Y, -DeltaPos.X).GetSafeNormal();

	// Come up with the final start/end points
	const FVector2D DirectionBias = Normal * LineSeparationAmount;
	const FVector2D LengthBias = ArrowRadius.X * UnitDelta;
	const FVector2D StartPoint = ArrowStartPoint + DirectionBias + LengthBias;
	const FVector2D EndPoint = ArrowEndPoint + DirectionBias - LengthBias;

	// Draw a line/spline
	DrawConnection(WireLayerID, StartPoint, EndPoint, Params);

	// Draw the arrow
	const FVector2D ArrowDrawPos = EndPoint - ArrowRadius;
	const float AngleInRadians = FMath::Atan2(DeltaPos.Y, DeltaPos.X);

	FSlateDrawElement::MakeRotatedBox(
		DrawElementsList,
		ArrowLayerID,
		FPaintGeometry(ArrowDrawPos, ArrowImage->ImageSize * ZoomFactor, ZoomFactor),
		ArrowImage,
		ESlateDrawEffect::None,
		AngleInRadians,
		TOptional<FVector2D>(),
		FSlateDrawElement::RelativeToElement,
		Params.WireColor
	);
}

void FConnectionDrawingPolicy_HoudiniBuildSequenceGraph::DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin)
{
	FConnectionParams Params;
	DetermineWiringStyle(Pin, nullptr, /*inout*/ Params);

	if (Pin->Direction == EEdGraphPinDirection::EGPD_Output)
	{
		DrawSplineWithArrow(FGeometryHelper::FindClosestPointOnGeom(PinGeometry, EndPoint), EndPoint, Params);
	}
	else
	{
		DrawSplineWithArrow(FGeometryHelper::FindClosestPointOnGeom(PinGeometry, StartPoint), StartPoint, Params);
	}
}

FVector2D FConnectionDrawingPolicy_HoudiniBuildSequenceGraph::ComputeSplineTangent(const FVector2D& Start, const FVector2D& End) const
{
	const FVector2D Delta = End - Start;
	const FVector2D NormDelta = Delta.GetSafeNormal();

	return NormDelta;
}

void FConnectionDrawingPolicy_HoudiniBuildSequenceGraph::DetermineLinkGeometry(FArrangedChildren& ArrangedNodes, TSharedRef<SWidget>& OutputPinWidget,
	UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FArrangedWidget*& StartWidgetGeometry, FArrangedWidget*& EndWidgetGeometry)
{
	if (auto* EdgeNode = Cast<UEdNode_HoudiniBuildSequenceEdge>(InputPin->GetOwningNode()))
	{
		UEdNode_HoudiniBuildSequenceNode* Start = EdgeNode->GetStartNode();
		UEdNode_HoudiniBuildSequenceNode* End = EdgeNode->GetEndNode();
		if (Start != nullptr && End != nullptr)
		{
			int32* StartNodeIndex = NodeWidgetMap.Find(Start);
			int32* EndNodeIndex = NodeWidgetMap.Find(End);
			if (StartNodeIndex != nullptr && EndNodeIndex != nullptr)
			{
				StartWidgetGeometry = &(ArrangedNodes[*StartNodeIndex]);
				EndWidgetGeometry = &(ArrangedNodes[*EndNodeIndex]);
			}
		}
	}
	else
	{
		StartWidgetGeometry = PinGeometries->Find(OutputPinWidget);

		if (TSharedPtr<SGraphPin>* pTargetWidget = PinToPinWidgetMap.Find(InputPin))
		{
			TSharedRef<SGraphPin> InputWidget = (*pTargetWidget).ToSharedRef();
			EndWidgetGeometry = PinGeometries->Find(InputWidget);
		}
	}
}

EGraphType UEdGraphSchema_HoudiniBuildSequenceGraph::GetGraphType(const UEdGraph* TestEdGraph) const
{
	return GT_StateMachine;
}

void UEdGraphSchema_HoudiniBuildSequenceGraph::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	auto* SequenceGraph = CastChecked<UHoudiniBuildSequenceGraph>(ContextMenuBuilder.CurrentGraph->GetOuter());

	if (SequenceGraph->GetSupportedNodeInfo().Num() == 0)
	{
		UE_LOG(LogEHEEditor, Warning, TEXT("warning: UEdGraphSchema_HoudiniBuildSequenceGraph::GetGraphContextActions(): expected at least one context action."));
	}

	for (FAutomationGraphSupportedNodeInfo NodeInfo : SequenceGraph->GetSupportedNodeInfo())
	{
		// You can only drag from output pins, not from input.
		if (ContextMenuBuilder.FromPin && ContextMenuBuilder.FromPin->Direction != EGPD_Output)
		{
			continue;
		}
		if (NodeInfo.NodeType->HasAnyClassFlags(CLASS_Abstract))
		{
			UE_LOG(LogEHEEditor, Warning, TEXT("warning: UEdGraphSchema_HoudiniBuildSequenceGraph::GetGraphContextActions(): found abstract class in list of supported node types"));
			continue;
		}

		// This is the DisplayName as specified by the class metadata tag. 
		const FText DisplayName = FText::FromString(NodeInfo.NodeType->GetDescription());

		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("Name"), DisplayName);
			const FText AddToolTip = FText::Format(LOCTEXT("NewHoudiniBuildSequenceNodeTooltip", "Adds {Name} node here"), Arguments);
		
			TSharedPtr<FAssetSchemaAction_HBSG_NewNode> NewNodeAction(new FAssetSchemaAction_HBSG_NewNode(
				NodeInfo.NewNodeMenuCategory,
				DisplayName,
				AddToolTip, 0)
			);
			NewNodeAction->NodeClass = NodeInfo.NodeType;
			ContextMenuBuilder.AddAction(NewNodeAction);
		}
	}
}

void UEdGraphSchema_HoudiniBuildSequenceGraph::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	// TODO(): This does not work correctly, because the user is likely to click on a "pin" that not linked to anything.
	//         since this node is "pinless", we need to check all pins, and if any are connected, we need to build the pin action submenu.
	/*
	if (Context->Pin)
	{
		FToolMenuSection& Section = Menu->AddSection("EdGraphSchema_HoudiniBuildSequenceGraphActions", LOCTEXT("PinActionsMenuHeader", "BSG Pin Actions"));
		// Only display the 'Break Links' option if there is a link to break!
		if (Context->Pin->LinkedTo.Num() > 0)
		{
			Section.AddMenuEntry(FGraphEditorCommands::Get().BreakPinLinks);

			// add sub menu for break link to
			if (Context->Pin->LinkedTo.Num() > 1)
			{
				Section.AddSubMenu(
					"BreakLinkTo",
					LOCTEXT("BreakLinkTo", "Break Link To..."),
					LOCTEXT("BreakSpecificLinks", "Break a specific link..."),
					FNewToolMenuDelegate::CreateUObject((UEdGraphSchema_HoudiniBuildSequenceGraph* const)this, &UEdGraphSchema_HoudiniBuildSequenceGraph::GetBreakLinkToSubMenuActions, const_cast<UEdGraphPin*>(Context->Pin)));
			}
			else
			{
				((UEdGraphSchema_HoudiniBuildSequenceGraph* const)this)->GetBreakLinkToSubMenuActions(Menu, const_cast<UEdGraphPin*>(Context->Pin));
			}
		}
	}*/
	
	if (Context->Pin || Context->Node)
	{
		FToolMenuSection& Section = Menu->AddSection("HBSG_ContextMenuActions", LOCTEXT("HBSG_ContextMenuActionHeader", "Node Actions"));
		Section.AddMenuEntry(FGenericCommands::Get().Rename);
		Section.AddMenuEntry(FGenericCommands::Get().Delete);
		Section.AddMenuEntry(FGenericCommands::Get().Cut);
		Section.AddMenuEntry(FGenericCommands::Get().Copy);
		Section.AddMenuEntry(FGenericCommands::Get().Duplicate);

		Section.AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);
	}
}

const FPinConnectionResponse UEdGraphSchema_HoudiniBuildSequenceGraph::CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	auto* FromEdNode = Cast<UEdNode_HoudiniBuildSequenceNode>(PinA->GetOwningNode());
	auto* ToEdNode = Cast<UEdNode_HoudiniBuildSequenceNode>(PinB->GetOwningNode());
	if (FromEdNode == nullptr || ToEdNode == nullptr)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinError_InvalidEdNode", "Not a valid UEdNode_HoudiniBuildSequenceNode"));
	}
	if (FromEdNode == ToEdNode)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionSameNode", "Can't connect a node to itself"));
	}

	TObjectPtr<UAutomationGraphNode> FromNode = FromEdNode->SequenceNode;
	TObjectPtr<UAutomationGraphNode> ToNode = ToEdNode->SequenceNode;
	if (FromEdNode == nullptr || ToEdNode == nullptr)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinError_InvalidNode", "Not a valid HoudiniBuildSequenceNode"));
	}

	if (FromNode->ChildNodes.Contains(ToNode) || ToNode->ChildNodes.Contains(FromNode))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinError_AlreadyConnected", "Can't connect nodes that are already connected"));
	}

	// Traverse FromNode and make sure that ToNode isn't one of its ancestors.
	TArray<TObjectPtr<UAutomationGraphNode>> NodeStack;
	TSet<TObjectPtr<UAutomationGraphNode>> Visited;
	
	NodeStack.Append(FromNode->ParentNodes);
	while (!NodeStack.IsEmpty())
	{
		TObjectPtr<UAutomationGraphNode> AncestorNode = NodeStack.Pop();

		if (!Visited.Contains(AncestorNode))
		{
			Visited.Add(AncestorNode);
			NodeStack.Append(AncestorNode->ParentNodes);
		}
	}

	if (Visited.Contains(ToNode))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinError_Cycle", "Can't create a graph cycle"));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, LOCTEXT("PinConnect", "Connect nodes with edge"));
}

bool UEdGraphSchema_HoudiniBuildSequenceGraph::TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	auto* FromEdNode = CastChecked<UEdNode_HoudiniBuildSequenceNode>(PinA->GetOwningNode());
	auto* ToEdNode = CastChecked<UEdNode_HoudiniBuildSequenceNode>(PinB->GetOwningNode());

	// We always connect output(A)-->input(B) regardless of which pin the user actually dragged off of.
	bool bModified = UEdGraphSchema::TryCreateConnection(FromEdNode->GetOutputPin(), ToEdNode->GetInputPin());

	if (bModified)
	{
		CastChecked<UEdGraph_HoudiniBuildSequenceGraph>(PinA->GetOwningNode()->GetGraph())->RebuildSequenceGraph();
	}

	return bModified;
}

bool UEdGraphSchema_HoudiniBuildSequenceGraph::CreateAutomaticConversionNodeAndConnections(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	auto* FromEdNode = Cast<UEdNode_HoudiniBuildSequenceNode>(PinA->GetOwningNode());
	auto* ToEdNode = Cast<UEdNode_HoudiniBuildSequenceNode>(PinB->GetOwningNode());

	// Are nodes and pins all valid?
	if (!FromEdNode || !FromEdNode->GetOutputPin() || !ToEdNode || !ToEdNode->GetInputPin())
	{
		return false;	
	}
	
	UEdGraph* Graph = FromEdNode->GetGraph();

	FVector2D InitPos((FromEdNode->NodePosX + ToEdNode->NodePosX) / 2, (FromEdNode->NodePosY + ToEdNode->NodePosY) / 2);

	FAssetSchemaAction_HBSG_NewEdge Action;
	auto* EdgeNode = Cast<UEdNode_HoudiniBuildSequenceEdge>(Action.PerformAction(FromEdNode->GetGraph(), nullptr, InitPos, false));
	EdgeNode->CreateConnections(FromEdNode, ToEdNode);

	return true;
}

FConnectionDrawingPolicy* UEdGraphSchema_HoudiniBuildSequenceGraph::CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj) const
{
	return new FConnectionDrawingPolicy_HoudiniBuildSequenceGraph(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
}

FLinearColor UEdGraphSchema_HoudiniBuildSequenceGraph::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FColor::White;
}

void UEdGraphSchema_HoudiniBuildSequenceGraph::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakNodeLinks", "Break Node Links"));

	Super::BreakNodeLinks(TargetNode);
	CastChecked<UEdGraph_HoudiniBuildSequenceGraph>(TargetNode.GetGraph())->RebuildSequenceGraph();
}

void UEdGraphSchema_HoudiniBuildSequenceGraph::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakPinLinks", "Break Pin Links"));

	Super::BreakPinLinks(TargetPin, bSendsNodeNotifcation);

	if (bSendsNodeNotifcation)
	{
		CastChecked<UEdGraph_HoudiniBuildSequenceGraph>(TargetPin.GetOwningNode()->GetGraph())->RebuildSequenceGraph();
	}
}

void UEdGraphSchema_HoudiniBuildSequenceGraph::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakSinglePinLink", "Break Pin Link"));

	Super::BreakSinglePinLink(SourcePin, TargetPin);
	CastChecked<UEdGraph_HoudiniBuildSequenceGraph>(SourcePin->GetOwningNode()->GetGraph())->RebuildSequenceGraph();
}

UEdGraphPin* UEdGraphSchema_HoudiniBuildSequenceGraph::DropPinOnNode(UEdGraphNode* InTargetNode, const FName& InSourcePinName, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection) const
{
	auto* EdNode = Cast<UEdNode_HoudiniBuildSequenceNode>(InTargetNode);
	switch (InSourcePinDirection)
	{
	case EGPD_Input:
		return EdNode->GetOutputPin();
	case EGPD_Output:
		return EdNode->GetInputPin();
	default:
		return nullptr;
	}
}

bool UEdGraphSchema_HoudiniBuildSequenceGraph::SupportsDropPinOnNode(UEdGraphNode* InTargetNode, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection, FText& OutErrorMessage) const
{
	return Cast<UEdNode_HoudiniBuildSequenceNode>(InTargetNode) != nullptr;
}

void UEdGraphSchema_HoudiniBuildSequenceGraph::GetBreakLinkToSubMenuActions(UToolMenu* SubMenu, UEdGraphPin* SelectedGraphPin)
{
	// Make sure we have a unique name for every entry in the list
	TMap< FString, uint32 > LinkTitleCount;

	FToolMenuSection& Section = SubMenu->FindOrAddSection("HoudiniBuildSequenceGraphSchemaPinActions");

	// Since this graph uses "pinless" nodes, we need to loop through all pins in order to get the full list of
	// connections that can be broken
	for (UEdGraphPin* GraphPin : SelectedGraphPin->GetOwningNode()->Pins)
	{
		// Add all the links we could break from
		for (TArray<class UEdGraphPin*>::TConstIterator Links(GraphPin->LinkedTo); Links; ++Links)
		{
			UEdGraphPin* Pin = *Links;
			FString TitleString = Pin->GetOwningNode()->GetNodeTitle(ENodeTitleType::ListView).ToString();
			FText Title = FText::FromString(TitleString);
			if (Pin->PinName != TEXT(""))
			{
				TitleString = FString::Printf(TEXT("%s (%s)"), *TitleString, *Pin->PinName.ToString());

				// Add name of connection if possible
				FFormatNamedArguments Args;
				Args.Add(TEXT("NodeTitle"), Title);
				Args.Add(TEXT("PinName"), Pin->GetDisplayName());
				Title = FText::Format(LOCTEXT("BreakDescPin", "{NodeTitle} ({PinName})"), Args);
			}

			uint32& Count = LinkTitleCount.FindOrAdd(TitleString);

			FText Description;
			FFormatNamedArguments Args;
			Args.Add(TEXT("NodeTitle"), Title);
			Args.Add(TEXT("NumberOfNodes"), Count);

			if (Count == 0)
			{
				Description = FText::Format(LOCTEXT("BreakDesc", "Break link to {NodeTitle}"), Args);
			}
			else
			{
				Description = FText::Format(LOCTEXT("BreakDescMulti", "Break link to {NodeTitle} ({NumberOfNodes})"), Args);
			}
			++Count;

			Section.AddMenuEntry(NAME_None, Description, Description, FSlateIcon(), FUIAction(
				FExecuteAction::CreateUObject(this, &UEdGraphSchema_HoudiniBuildSequenceGraph::BreakSinglePinLink, const_cast<UEdGraphPin*>(GraphPin), *Links)));
		}
	}
}

UHoudiniBuildSequenceGraph* UEdGraph_HoudiniBuildSequenceGraph::GetBuildSequenceGraph()
{
	return CastChecked<UHoudiniBuildSequenceGraph>(GetOuter());
}

void UEdGraph_HoudiniBuildSequenceGraph::RebuildSequenceGraph()
{
	UHoudiniBuildSequenceGraph* SequenceGraph = GetBuildSequenceGraph();
	
	SequenceGraph->RootNodes.Empty();
	
	for (TObjectPtr<UEdGraphNode> Node : Nodes)
	{
		auto* SequenceGraphNode = Cast<UEdNode_HoudiniBuildSequenceNode>(Node);
		if (!SequenceGraphNode)
		{
			UE_LOG(LogEHEEditor, Warning, TEXT("warning: UEdGraph_HoudiniBuildSequenceGraph::RebuildSequenceGraph(): Expected SequenceGraphNode to be valid"));
			continue;
		}

		TObjectPtr<UAutomationGraphNode> SequenceNode = SequenceGraphNode->SequenceNode;
		if (!SequenceNode)
		{
			UE_LOG(LogEHEEditor, Warning, TEXT("warning: UEdGraph_HoudiniBuildSequenceGraph::RebuildSequenceGraph(): Expected BuildSequenceNode to be valid"));
			continue;
		}

		SequenceNode->ParentNodes.Empty();
		SequenceNode->ChildNodes.Empty();

		// We pre-load the nodes into a set so we can check for duplicates.
		TSet<TObjectPtr<UAutomationGraphNode>> ParentNodes;
		TSet<TObjectPtr<UAutomationGraphNode>> ChildNodes;
		
		for (UEdGraphPin* Pin : SequenceGraphNode->Pins)
		{
			if (!Pin)
			{
				UE_LOG(LogEHEEditor, Error, TEXT("warning: UEdGraph_HoudiniBuildSequenceGraph::RebuildSequenceGraph(): Expected Pin to be valid"));
				continue;
			}

			for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				auto* GraphEdge = CastChecked<UEdNode_HoudiniBuildSequenceEdge>(LinkedPin->GetOwningNode());
				if (!GraphEdge)
				{
					UE_LOG(LogEHEEditor, Error, TEXT("warning: UEdGraph_HoudiniBuildSequenceGraph::RebuildSequenceGraph(): Expected Graph edge to be valid"));
					continue;
				}

				UEdNode_HoudiniBuildSequenceNode* LinkedSequenceGraphNode = nullptr;
				if (Pin->Direction == EGPD_Input)
				{
					LinkedSequenceGraphNode = GraphEdge->GetStartNode();
				}
				else if (Pin->Direction == EGPD_Output)
				{
					LinkedSequenceGraphNode = GraphEdge->GetEndNode();
				}
				if (!LinkedSequenceGraphNode)
				{
					UE_LOG(LogEHEEditor, Error, TEXT("warning: UEdGraph_HoudiniBuildSequenceGraph::RebuildSequenceGraph(): Expected linked graph node to be valid"));
					continue;
				}
				if (LinkedSequenceGraphNode == SequenceGraphNode)
				{
					UE_LOG(LogEHEEditor, Error, TEXT("warning: UEdGraph_HoudiniBuildSequenceGraph::RebuildSequenceGraph(): Expected linked graph node to be a different node"));
					continue;
				}
				
				if (Pin->Direction == EGPD_Input)
				{
					if (ParentNodes.Contains(LinkedSequenceGraphNode->SequenceNode))
					{
						UE_LOG(LogEHEEditor, Warning, TEXT("warning: UEdGraph_HoudiniBuildSequenceGraph::RebuildSequenceGraph(): Node has multiple connections to the same parent"));
						continue;
					}
					
					ParentNodes.Add(LinkedSequenceGraphNode->SequenceNode);
				}
				else if (Pin->Direction == EGPD_Output)
				{
					if (ChildNodes.Contains(LinkedSequenceGraphNode->SequenceNode))
					{
						UE_LOG(LogEHEEditor, Warning, TEXT("warning: UEdGraph_HoudiniBuildSequenceGraph::RebuildSequenceGraph(): Node has multiple connections to the same child"));
						continue;
					}
					
					ChildNodes.Add(LinkedSequenceGraphNode->SequenceNode);
				}
			}
		}

		for (TObjectPtr<UAutomationGraphNode> ParentNode : ParentNodes)
		{
			SequenceNode->ParentNodes.Add(ParentNode);
		}
		for (TObjectPtr<UAutomationGraphNode> ChildNode : ChildNodes)
		{
			SequenceNode->ChildNodes.Add(ChildNode);
		}
		
		if (SequenceNode->ParentNodes.IsEmpty())
		{
			SequenceGraph->RootNodes.Add(SequenceNode);
		}
	}
}

#undef LOCTEXT_NAMESPACE