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

#include "EdGraph/EdNode_HoudiniBuildSequenceEdge.h"

#include "ConnectionDrawingPolicy.h"
#include "EdGraph/EdNode_HoudiniBuildSequenceNode.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

void SEdNode_HoudiniBuildSequenceEdge::Construct(const FArguments& InArgs, UEdNode_HoudiniBuildSequenceEdge* InNode)
{
	GraphNode = InNode;
	UpdateGraphNode();
}

void SEdNode_HoudiniBuildSequenceEdge::PerformSecondPassLayout(const TMap<UObject*, TSharedRef<SNode>>& NodeToWidgetLookup) const
{
	auto* EdgeNode = CastChecked<UEdNode_HoudiniBuildSequenceEdge>(GraphNode);

	FGeometry StartGeom;
	FGeometry EndGeom;

	UEdNode_HoudiniBuildSequenceNode* Start = EdgeNode->GetStartNode();
	UEdNode_HoudiniBuildSequenceNode* End = EdgeNode->GetEndNode();
	if (Start != nullptr && End != nullptr)
	{
		const TSharedRef<SNode>* pFromWidget = NodeToWidgetLookup.Find(Start);
		const TSharedRef<SNode>* pToWidget = NodeToWidgetLookup.Find(End);
		if (pFromWidget != nullptr && pToWidget != nullptr)
		{
			const TSharedRef<SNode>& FromWidget = *pFromWidget;
			const TSharedRef<SNode>& ToWidget = *pToWidget;

			StartGeom = FGeometry(FVector2D(Start->NodePosX, Start->NodePosY), FVector2D::ZeroVector, FromWidget->GetDesiredSize(), 1.0f);
			EndGeom = FGeometry(FVector2D(End->NodePosX, End->NodePosY), FVector2D::ZeroVector, ToWidget->GetDesiredSize(), 1.0f);
		}
	}

	PositionBetweenTwoNodesWithOffset(StartGeom, EndGeom, 0, 1);
}

void SEdNode_HoudiniBuildSequenceEdge::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);
	
	this->ContentScale.Bind( this, &SGraphNode::GetContentScale );
	
	this->GetOrAddSlot( ENodeZone::Center )
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("Graph.TransitionNode.ColorSpill"))
				.ColorAndOpacity(this, &SEdNode_HoudiniBuildSequenceEdge::GetEdgeColor)
			]
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(this, &SEdNode_HoudiniBuildSequenceEdge::GetEdgeImage)
				.Visibility(this, &SEdNode_HoudiniBuildSequenceEdge::GetEdgeImageVisibility)
			]

			+ SOverlay::Slot()
			.Padding(FMargin(4.0f, 4.0f, 4.0f, 4.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.AutoHeight()
				[
					SAssignNew(InlineEditableText, SInlineEditableTextBlock)
					.ColorAndOpacity(FLinearColor::Black)
					.Visibility(this, &SEdNode_HoudiniBuildSequenceEdge::GetEdgeTitleVisbility)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
					.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
					.OnTextCommitted(this, &SEdNode_HoudiniBuildSequenceEdge::OnNameTextCommited)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					NodeTitle.ToSharedRef()
				]
			]
		];
}

void SEdNode_HoudiniBuildSequenceEdge::PositionBetweenTwoNodesWithOffset(const FGeometry& StartGeom, const FGeometry& EndGeom, int32 NodeIndex, int32 MaxNodes) const
{
	// Get a reasonable seed point (halfway between the boxes)
	const FVector2D StartCenter = FGeometryHelper::CenterOf(StartGeom);
	const FVector2D EndCenter = FGeometryHelper::CenterOf(EndGeom);
	const FVector2D SeedPoint = (StartCenter + EndCenter) * 0.5f;

	// Find the (approximate) closest points between the two boxes
	const FVector2D StartAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(StartGeom, SeedPoint);
	const FVector2D EndAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(EndGeom, SeedPoint);

	// Position ourselves halfway along the connecting line between the nodes, elevated away perpendicular to the direction of the line
	const float Height = 30.0f;

	const FVector2D DesiredNodeSize = GetDesiredSize();

	FVector2D DeltaPos(EndAnchorPoint - StartAnchorPoint);

	if (DeltaPos.IsNearlyZero())
	{
		DeltaPos = FVector2D(10.0f, 0.0f);
	}

	const FVector2D Normal = FVector2D(DeltaPos.Y, -DeltaPos.X).GetSafeNormal();

	const FVector2D NewCenter = StartAnchorPoint + (0.5f * DeltaPos) + (Height * Normal);

	FVector2D DeltaNormal = DeltaPos.GetSafeNormal();
	
	// Calculate node offset in the case of multiple transitions between the same two nodes
	// MultiNodeOffset: the offset where 0 is the centre of the transition, -1 is 1 <size of node>
	// towards the PrevStateNode and +1 is 1 <size of node> towards the NextStateNode.

	const float MutliNodeSpace = 0.2f; // Space between multiple transition nodes (in units of <size of node> )
	const float MultiNodeStep = (1.f + MutliNodeSpace); //Step between node centres (Size of node + size of node spacer)

	const float MultiNodeStart = -((MaxNodes - 1) * MultiNodeStep) / 2.f;
	const float MultiNodeOffset = MultiNodeStart + (NodeIndex * MultiNodeStep);

	// Now we need to adjust the new center by the node size, zoom factor and multi node offset
	const FVector2D NewCorner = NewCenter - (0.5f * DesiredNodeSize) + (DeltaNormal * MultiNodeOffset * DesiredNodeSize.Size());

	GraphNode->NodePosX = NewCorner.X;
	GraphNode->NodePosY = NewCorner.Y;
}

FSlateColor SEdNode_HoudiniBuildSequenceEdge::GetEdgeColor() const
{
	return FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
}

const FSlateBrush* SEdNode_HoudiniBuildSequenceEdge::GetEdgeImage() const
{
	return FAppStyle::GetBrush("Graph.TransitionNode.Icon");
	// return FAppStyle::GetBrush("BreadcrumbTrail.Delimiter");
}

EVisibility SEdNode_HoudiniBuildSequenceEdge::GetEdgeImageVisibility() const
{
	return EVisibility::Visible;
}

EVisibility SEdNode_HoudiniBuildSequenceEdge::GetEdgeTitleVisbility() const
{
	return EVisibility::Collapsed;
}

void UEdNode_HoudiniBuildSequenceEdge::AllocateDefaultPins()
{
	UEdGraphPin* Inputs = CreatePin(EGPD_Input, TEXT("Edge"), FName(), TEXT("In"));
	Inputs->bHidden = true;
	UEdGraphPin* Outputs = CreatePin(EGPD_Output, TEXT("Edge"), FName(), TEXT("Out"));
	Outputs->bHidden = true;
}

FText UEdNode_HoudiniBuildSequenceEdge::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString("Edge");
}

void UEdNode_HoudiniBuildSequenceEdge::PinConnectionListChanged(UEdGraphPin* Pin)
{
	if (Pin->LinkedTo.Num() == 0)
	{
		Modify();
		if (UEdGraph* ParentGraph = GetGraph())
		{
			ParentGraph->Modify();
		}

		DestroyNode();
	}
}

void UEdNode_HoudiniBuildSequenceEdge::CreateConnections(UEdNode_HoudiniBuildSequenceNode* Start, UEdNode_HoudiniBuildSequenceNode* End)
{
	Pins[0]->Modify();
	Pins[0]->LinkedTo.Empty();

	Start->GetOutputPin()->Modify();
	Pins[0]->MakeLinkTo(Start->GetOutputPin());

	// This to next
	Pins[1]->Modify();
	Pins[1]->LinkedTo.Empty();

	End->GetInputPin()->Modify();
	Pins[1]->MakeLinkTo(End->GetInputPin());
}

UEdNode_HoudiniBuildSequenceNode* UEdNode_HoudiniBuildSequenceEdge::GetStartNode()
{
	if (Pins[0]->LinkedTo.Num() > 0)
	{
		return Cast<UEdNode_HoudiniBuildSequenceNode>(Pins[0]->LinkedTo[0]->GetOwningNode());
	}
	else
	{
		return nullptr;
	}
}

UEdNode_HoudiniBuildSequenceNode* UEdNode_HoudiniBuildSequenceEdge::GetEndNode()
{
	if (Pins[1]->LinkedTo.Num() > 0)
	{
		return Cast<UEdNode_HoudiniBuildSequenceNode>(Pins[1]->LinkedTo[0]->GetOwningNode());
	}
	else
	{
		return nullptr;
	}
}