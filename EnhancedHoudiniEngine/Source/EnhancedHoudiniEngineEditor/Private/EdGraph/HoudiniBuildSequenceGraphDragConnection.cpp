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

#include "EdGraph/HoudiniBuildSequenceGraphDragConnection.h"
#include "EdGraph/EdNode_HoudiniBuildSequenceNode.h"
#include "Widgets/SBoxPanel.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Images/SImage.h"
#include "EdGraph/EdGraph.h"
#include "SGraphPanel.h"
#include "ScopedTransaction.h"

TSharedRef<FHoudiniBuildSequenceGraphDragConnection> FHoudiniBuildSequenceGraphDragConnection::New(const TSharedRef<SGraphPanel>& GraphPanel, const FDraggedPinTable& DraggedPins)
{
	TSharedRef<FHoudiniBuildSequenceGraphDragConnection> Operation = MakeShareable(new FHoudiniBuildSequenceGraphDragConnection(GraphPanel, DraggedPins));
	Operation->Construct();

	return Operation;
}

void FHoudiniBuildSequenceGraphDragConnection::OnDrop(bool bDropWasHandled, const FPointerEvent& MouseEvent)
{
	GraphPanel->OnStopMakingConnection();

	Super::OnDrop(bDropWasHandled, MouseEvent);
}

void FHoudiniBuildSequenceGraphDragConnection::OnDragged(const class FDragDropEvent& DragDropEvent)
{
	FVector2D TargetPosition = DragDropEvent.GetScreenSpacePosition();

	// Reposition the info window wrt to the drag
	CursorDecoratorWindow->MoveWindowTo(DragDropEvent.GetScreenSpacePosition() + DecoratorAdjust);
	// Request the active panel to scroll if required
	GraphPanel->RequestDeferredPan(TargetPosition);
}

void FHoudiniBuildSequenceGraphDragConnection::HoverTargetChanged()
{
	TArray<FPinConnectionResponse> UniqueMessages;

	if (UEdGraphPin* TargetPinObj = GetHoveredPin())
	{
		TArray<UEdGraphPin*> ValidSourcePins;
		ValidateGraphPinList(/*out*/ ValidSourcePins);

		// Check the schema for connection responses
		for (UEdGraphPin* StartingPinObj : ValidSourcePins)
		{
			// The Graph object in which the pins reside.
			UEdGraph* GraphObj = StartingPinObj->GetOwningNode()->GetGraph();

			// Determine what the schema thinks about the wiring action
			const FPinConnectionResponse Response = GraphObj->GetSchema()->CanCreateConnection(StartingPinObj, TargetPinObj);

			if (Response.Response == ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW)
			{
				TSharedPtr<SGraphNode> NodeWidget = TargetPinObj->GetOwningNode()->DEPRECATED_NodeWidget.Pin();
				if (NodeWidget.IsValid())
				{
					NodeWidget->NotifyDisallowedPinConnection(StartingPinObj, TargetPinObj);
				}
			}

			UniqueMessages.AddUnique(Response);
		}
	}
	else if (auto* TargetNodeObj = Cast<UEdNode_HoudiniBuildSequenceNode>(GetHoveredNode()))
	{
		TArray<UEdGraphPin*> ValidSourcePins;
		ValidateGraphPinList(/*out*/ ValidSourcePins);

		// Check the schema for connection responses
		for (UEdGraphPin* StartingPinObj : ValidSourcePins)
		{
			FPinConnectionResponse Response;			
			FText ResponseText;

			const UEdGraphSchema *Schema = StartingPinObj->GetSchema();
			UEdGraphPin *TargetPin = TargetNodeObj->GetInputPin();

			if (Schema && TargetPin)
			{
				Response = Schema->CanCreateConnection(StartingPinObj, TargetPin);
				if (Response.Response == ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW)
				{
					TSharedPtr<SGraphNode> NodeWidget = TargetPin->GetOwningNode()->DEPRECATED_NodeWidget.Pin();
					if (NodeWidget.IsValid())
					{
						NodeWidget->NotifyDisallowedPinConnection(StartingPinObj, TargetPinObj);
					}
				}
			}
			else
			{
#define LOCTEXT_NAMESPACE "AssetSchema_HoudiniBuildSequenceGraph"
				Response = FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinError", "Not a valid UEdNode_HoudiniBuildSequenceNode"));
#undef LOCTEXT_NAMESPACE
			}

			UniqueMessages.AddUnique(Response);
		}
	}
	else if (UEdGraph* CurrentHoveredGraph = GetHoveredGraph())
	{
		TArray<UEdGraphPin*> ValidSourcePins;
		ValidateGraphPinList(/*out*/ ValidSourcePins);

		for (UEdGraphPin* StartingPinObj : ValidSourcePins)
		{
			// Let the schema describe the connection we might make
			FPinConnectionResponse Response = CurrentHoveredGraph->GetSchema()->CanCreateNewNodes(StartingPinObj);
			if (!Response.Message.IsEmpty())
			{
				UniqueMessages.AddUnique(Response);
			}
		}
	}

	// Let the user know the status of dropping now
	if (UniqueMessages.Num() == 0)
	{
		// Display the place a new node icon, we're not over a valid pin and have no message from the schema
		SetSimpleFeedbackMessage(
			FAppStyle::GetBrush(TEXT("Graph.ConnectorFeedback.NewNode")),
			FLinearColor::White,
			NSLOCTEXT("GraphEditor.Feedback", "PlaceNewNode", "Place a new node."));
	}
	else
	{
		// Take the unique responses and create visual feedback for it
		TSharedRef<SVerticalBox> FeedbackBox = SNew(SVerticalBox);
		for (auto ResponseIt = UniqueMessages.CreateConstIterator(); ResponseIt; ++ResponseIt)
		{
			// Determine the icon
			const FSlateBrush* StatusSymbol = NULL;

			switch (ResponseIt->Response)
			{
			case CONNECT_RESPONSE_MAKE:
			case CONNECT_RESPONSE_BREAK_OTHERS_A:
			case CONNECT_RESPONSE_BREAK_OTHERS_B:
			case CONNECT_RESPONSE_BREAK_OTHERS_AB:
				StatusSymbol = FAppStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK"));
				break;

			case CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE:
				StatusSymbol = FAppStyle::GetBrush(TEXT("Graph.ConnectorFeedback.ViaCast"));
				break;

			case CONNECT_RESPONSE_DISALLOW:
			default:
				StatusSymbol = FAppStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error"));
				break;
			}

			// Add a new message row
			FeedbackBox->AddSlot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(3.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SImage).Image(StatusSymbol)
				]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(ResponseIt->Message)
				]
				];
		}

		SetFeedbackMessage(FeedbackBox);
	}
}

FHoudiniBuildSequenceGraphDragConnection::FHoudiniBuildSequenceGraphDragConnection(const TSharedRef<SGraphPanel>& GraphPanelIn, const FDraggedPinTable& DraggedPinsIn)
	: GraphPanel(GraphPanelIn)
	, DraggingPins(DraggedPinsIn)
	, DecoratorAdjust(FSlateApplication::Get().GetCursorSize())
{
	if (DraggingPins.Num() > 0)
	{
		const UEdGraphPin* PinObj = FDraggedPinTable::TConstIterator(DraggedPinsIn)->GetPinObj(*GraphPanelIn);
		if (PinObj && PinObj->Direction == EGPD_Input)
		{
			DecoratorAdjust *= FVector2D(-1.0f, 1.0f);
		}
	}

	for (const FGraphPinHandle& DraggedPin : DraggedPinsIn)
	{
		GraphPanelIn->OnBeginMakingConnection(DraggedPin);
	}
}

FReply FHoudiniBuildSequenceGraphDragConnection::DroppedOnPin(FVector2D ScreenPosition, FVector2D GraphPosition)
{
	TArray<UEdGraphPin*> ValidSourcePins;
	ValidateGraphPinList(/*out*/ ValidSourcePins);

	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_CreateConnection", "Create Pin Link"));

	UEdGraphPin* PinB = GetHoveredPin();
	bool bError = false;
	TSet<UEdGraphNode*> NodeList;

	for (UEdGraphPin* PinA : ValidSourcePins)
	{
		if ((PinA != NULL) && (PinB != NULL))
		{
			UEdGraph* MyGraphObj = PinA->GetOwningNode()->GetGraph();

			if (MyGraphObj->GetSchema()->TryCreateConnection(PinA, PinB))
			{
				if (!PinA->IsPendingKill())
				{
					NodeList.Add(PinA->GetOwningNode());
				}
				if (!PinB->IsPendingKill())
				{
					NodeList.Add(PinB->GetOwningNode());
				}
			}
		}
		else
		{
			bError = true;
		}
	}

	// Send all nodes that received a new pin connection a notification
	for (auto It = NodeList.CreateConstIterator(); It; ++It)
	{
		UEdGraphNode* Node = (*It);
		Node->NodeConnectionListChanged();
	}

	if (bError)
	{
		return FReply::Unhandled();
	}

	return FReply::Handled();
}

FReply FHoudiniBuildSequenceGraphDragConnection::DroppedOnNode(FVector2D ScreenPosition, FVector2D GraphPosition)
{
	bool bHandledPinDropOnNode = false;
	UEdGraphNode* NodeOver = GetHoveredNode();

	if (NodeOver)
	{
		// Gather any source drag pins
		TArray<UEdGraphPin*> ValidSourcePins;
		ValidateGraphPinList(/*out*/ ValidSourcePins);

		if (ValidSourcePins.Num())
		{
			for (UEdGraphPin* SourcePin : ValidSourcePins)
			{
				// Check for pin drop support
				FText ResponseText;
				if (SourcePin->GetOwningNode() != NodeOver && SourcePin->GetSchema()->SupportsDropPinOnNode(NodeOver, SourcePin->PinType, SourcePin->Direction, ResponseText))
				{
					bHandledPinDropOnNode = true;

					// Find which pin name to use and drop the pin on the node
					const FName PinName = SourcePin->PinFriendlyName.IsEmpty() ? SourcePin->PinName : *SourcePin->PinFriendlyName.ToString();

					const FScopedTransaction Transaction((SourcePin->Direction == EGPD_Output) ? NSLOCTEXT("UnrealEd", "AddInParam", "Add In Parameter") : NSLOCTEXT("UnrealEd", "AddOutParam", "Add Out Parameter"));

					UEdGraphPin* EdGraphPin = NodeOver->GetSchema()->DropPinOnNode(GetHoveredNode(), PinName, SourcePin->PinType, SourcePin->Direction);

					// This can invalidate the source pin due to node reconstruction, abort in that case
					if (SourcePin->GetOwningNodeUnchecked() && EdGraphPin)
					{
						SourcePin->Modify();
						EdGraphPin->Modify();
						SourcePin->GetSchema()->TryCreateConnection(SourcePin, EdGraphPin);
					}
				}

				// If we have not handled the pin drop on node and there is an error message, do not let other actions occur.
				if (!bHandledPinDropOnNode && !ResponseText.IsEmpty())
				{
					bHandledPinDropOnNode = true;
				}
			}
		}
	}
	return bHandledPinDropOnNode ? FReply::Handled() : FReply::Unhandled();
}

FReply FHoudiniBuildSequenceGraphDragConnection::DroppedOnPanel(const TSharedRef< SWidget >& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph)
{
	// Gather any source drag pins
	TArray<UEdGraphPin*> PinObjects;
	ValidateGraphPinList(/*out*/ PinObjects);

	// Create a context menu
	TSharedPtr<SWidget> WidgetToFocus = GraphPanel->SummonContextMenu(ScreenPosition, GraphPosition, NULL, NULL, PinObjects);

	// Give the context menu focus
	return (WidgetToFocus.IsValid())
		? FReply::Handled().SetUserFocus(WidgetToFocus.ToSharedRef(), EFocusCause::SetDirectly)
		: FReply::Handled();
}


void FHoudiniBuildSequenceGraphDragConnection::ValidateGraphPinList(TArray<UEdGraphPin*>& OutValidPins)
{
	OutValidPins.Empty(DraggingPins.Num());
	for (const FGraphPinHandle& PinHandle : DraggingPins)
	{
		if (UEdGraphPin* GraphPin = PinHandle.GetPinObj(*GraphPanel))
		{
			OutValidPins.Add(GraphPin);
		}
	}
}
