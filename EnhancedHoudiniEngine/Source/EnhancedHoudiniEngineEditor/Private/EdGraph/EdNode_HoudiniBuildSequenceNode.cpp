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

#include "EdGraph\EdNode_HoudiniBuildSequenceNode.h"

#include "AutomationNodes/ClearLandscapeLayersNode.h"
#include "EdGraph/HoudiniBuildSequenceGraphDragConnection.h"
#include "EdGraph/HoudiniBuildSequenceGraphEditorTypes.h"
#include "Editor/HoudiniBuildSequenceGraphEditorStyle.h"
#include "EHEEditorLoggingDefs.h"
#include "Foundation/HoudiniBuildSequenceNode.h"
#include "GraphEditorSettings.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "SCommentBubble.h"
#include "SGraphPin.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

#define LOCTEXT_NAMESPACE "EdNode_HoudiniBuildSequenceNode"

class SHoudiniBuildSequenceNodeGraphPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SHoudiniBuildSequenceNodeGraphPin) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin)
	{
		this->SetCursor(EMouseCursor::Default);

		bShowLabel = true;

		GraphPinObj = InPin;
		check(GraphPinObj != nullptr);

		const UEdGraphSchema* Schema = GraphPinObj->GetSchema();
		check(Schema);

		SBorder::Construct(SBorder::FArguments()
			.BorderImage(this, &SHoudiniBuildSequenceNodeGraphPin::GetPinBorder)
			.BorderBackgroundColor(this, &SHoudiniBuildSequenceNodeGraphPin::GetPinColor)
			.OnMouseButtonDown(this, &SHoudiniBuildSequenceNodeGraphPin::OnPinMouseDown)
			.Cursor(this, &SHoudiniBuildSequenceNodeGraphPin::GetPinCursor)
			.Padding(FMargin(5.0f))
		);
	}

protected:
	// TODO(): Delete me?
	/*
	virtual FSlateColor GetPinColor() const override
	{
		return FLinearColor(0.02f, 0.02f, 0.02f);
	}*/

	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override
	{
		return SNew(STextBlock);
	}

	const FSlateBrush* GetPinBorder() const
	{
		// TODO(): Delete me?
		// return FAppStyle::GetBrush(TEXT("Graph.StateNode.Body"));
		return ( IsHovered() )
		? FAppStyle::GetBrush( TEXT("Graph.StateNode.Pin.BackgroundHovered") )
		: FAppStyle::GetBrush( TEXT("Graph.StateNode.Pin.Background") );

		// return FHoudiniBuildSequenceGraphEditorStyle::Get()->GetBrush(TEXT("HoudiniBuildSequenceGraphEditor.NodeBorder.Invisible"));
		
		/*
		if (IsHovered())
		{
			auto Current = BorderImage;
			return FHoudiniBuildSequenceGraphEditorStyle::Get()->GetBrush(TEXT("HoudiniBuildSequenceGraphEditor.NodeBorder.DefaultHovered"));
		}
		
		return FHoudiniBuildSequenceGraphEditorStyle::Get()->GetBrush(TEXT("HoudiniBuildSequenceGraphEditor.NodeBorder.Default"));*/
	}

	virtual TSharedRef<FDragDropOperation> SpawnPinDragEvent(const TSharedRef<class SGraphPanel>& InGraphPanel, const TArray< TSharedRef<SGraphPin> >& InStartingPins) override
	{
		FHoudiniBuildSequenceGraphDragConnection::FDraggedPinTable PinHandles;
		PinHandles.Reserve(InStartingPins.Num());
		// since the graph can be refreshed and pins can be reconstructed/replaced 
		// behind the scenes, the DragDropOperation holds onto FGraphPinHandles 
		// instead of direct widgets/graph-pins
		for (const TSharedRef<SGraphPin>& PinWidget : InStartingPins)
		{
			PinHandles.Add(PinWidget->GetPinObj());
		}

		return FHoudiniBuildSequenceGraphDragConnection::New(InGraphPanel, PinHandles);
	}

};

void SEdNode_HoudiniBuildSequenceNode::Construct(const FArguments& InArgs, UEdNode_HoudiniBuildSequenceNode* InNode)
{
	GraphNode = InNode;
	UpdateGraphNode();
}

void SEdNode_HoudiniBuildSequenceNode::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();
	
	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	const FSlateBrush* NodeTypeIcon = GetNameIcon();

	FLinearColor TitleShadowColor(0.6f, 0.6f, 0.6f);
	TSharedPtr<SErrorText> ErrorText;
	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

	this->ContentScale.Bind( this, &SGraphNode::GetContentScale );
	this->GetOrAddSlot( ENodeZone::Center )
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage( FAppStyle::GetBrush( "Graph.StateNode.Body" ) )
			.Padding(0)
			.BorderBackgroundColor( this, &SEdNode_HoudiniBuildSequenceNode::GetBorderBackgroundColor )
			[
				SNew(SOverlay)

				// PIN AREA
				+SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					// Only the output "pin" is selectable.
					SAssignNew(RightNodeBox, SVerticalBox)
				]

				// STATE NAME AREA
				+SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(10.0f)
				[
					SNew(SBorder)
					.BorderImage( FAppStyle::GetBrush("Graph.StateNode.ColorSpill") )
					.BorderBackgroundColor( TitleShadowColor )
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(10.0f)
					.Visibility(EVisibility::SelfHitTestInvisible)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							// POPUP ERROR MESSAGE
							SAssignNew(ErrorText, SErrorText )
							.BackgroundColor( this, &SEdNode_HoudiniBuildSequenceNode::GetErrorColor )
							.ToolTipText( this, &SEdNode_HoudiniBuildSequenceNode::GetErrorMsgToolTip )
						]
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SImage)
							.Image(NodeTypeIcon)
						]
						+SHorizontalBox::Slot()
						.Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
								.AutoHeight()
							[
								SAssignNew(InlineEditableText, SInlineEditableTextBlock)
								.Style( FAppStyle::Get(), "Graph.StateNode.NodeTitleInlineEditableText" )
								.Text( NodeTitle.Get(), &SNodeTitle::GetHeadTitle )
								.OnVerifyTextChanged(this, &SEdNode_HoudiniBuildSequenceNode::OnVerifyNameTextChanged)
								.OnTextCommitted(this, &SEdNode_HoudiniBuildSequenceNode::OnNameTextCommited)
								.IsReadOnly( this, &SEdNode_HoudiniBuildSequenceNode::IsNameReadOnly )
								.IsSelected(this, &SEdNode_HoudiniBuildSequenceNode::IsSelectedExclusively)
							]
							+SVerticalBox::Slot()
								.AutoHeight()
							[
								NodeTitle.ToSharedRef()
							]
						]
					]
				]
			]
		];

	ErrorReporting = ErrorText;
	ErrorReporting->SetError(ErrorMsg);
	CreatePinWidgets();
}

void SEdNode_HoudiniBuildSequenceNode::CreatePinWidgets()
{
	auto* MyNode = CastChecked<UEdNode_HoudiniBuildSequenceNode>(GraphNode);

	for (int32 PinIdx = 0; PinIdx < MyNode->Pins.Num(); PinIdx++)
	{
		UEdGraphPin* MyPin = MyNode->Pins[PinIdx];
		if (!MyPin->bHidden)
		{
			TSharedPtr<SGraphPin> NewPin = SNew(SHoudiniBuildSequenceNodeGraphPin, MyPin);
			
			AddPin(NewPin.ToSharedRef());
		}
	}
}

void SEdNode_HoudiniBuildSequenceNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));

	const UEdGraphPin* PinObj = PinToAdd->GetPinObj();
	const bool bAdvancedParameter = PinObj && PinObj->bAdvancedView;
	if (bAdvancedParameter)
	{
		PinToAdd->SetVisibility( TAttribute<EVisibility>(PinToAdd, &SGraphPin::IsPinVisibleAsAdvanced) );
	}

	TSharedPtr<SVerticalBox> PinBox;
	if (PinToAdd->GetDirection() == EEdGraphPinDirection::EGPD_Input)
	{
		PinBox = LeftNodeBox;
		InputPins.Add(PinToAdd);
	}
	else // Direction == EEdGraphPinDirection::EGPD_Output
	{
		PinBox = RightNodeBox;
		OutputPins.Add(PinToAdd);
	}

	if (PinBox)
	{
		PinBox->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.FillHeight(1.0f)
			//.Padding(6.0f, 0.0f)
			[
				PinToAdd
			];
	}
}

void SEdNode_HoudiniBuildSequenceNode::GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const
{
	auto* MyNode = CastChecked<UEdNode_HoudiniBuildSequenceNode>(GraphNode);

	if (!MyNode || !MyNode->SequenceNode)
	{
		return;
	}

	FLinearColor MessageColor = FLinearColor(1.f, 0.5f, 0.25f);
	FString NodeMessage = MyNode->SequenceNode->GetMessageText();

	if (!NodeMessage.IsEmpty())
	{
		Popups.Emplace(nullptr, MessageColor, NodeMessage);
	}
}

FSlateColor SEdNode_HoudiniBuildSequenceNode::GetBorderBackgroundColor() const
{
	auto* MyNode = CastChecked<UEdNode_HoudiniBuildSequenceNode>(GraphNode);
	
	FLinearColor StateColor_Inactive(0.08f, 0.08f, 0.08f);

	if (MyNode->SequenceNode)
	{
		return MyNode->SequenceNode->GetStateColor();
	}

	return StateColor_Inactive;
}

const FSlateBrush* SEdNode_HoudiniBuildSequenceNode::GetNameIcon() const
{
	auto* MyNode = CastChecked<UEdNode_HoudiniBuildSequenceNode>(GraphNode);
	return MyNode->GetNodeIcon();
}

void SEdNode_HoudiniBuildSequenceNode::OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo)
{
	SGraphNode::OnNameTextCommited(InText, CommitInfo);

	auto* MyNode = CastChecked<UEdNode_HoudiniBuildSequenceNode>(GraphNode);

	if (MyNode != nullptr && MyNode->SequenceNode != nullptr)
	{
		const FScopedTransaction Transaction(LOCTEXT("HoudiniBuildSequenceNodeRenameNode", "Houdini Build Sequence Node: Rename Node"));
		MyNode->Modify();
		MyNode->SequenceNode->Modify();
		MyNode->SequenceNode->Title = InText;
		UpdateGraphNode();
	}
}

UEdNode_HoudiniBuildSequenceNode::UEdNode_HoudiniBuildSequenceNode(const FObjectInitializer& Initializer): Super(Initializer)
{
	bCanRenameNode = true;
}

void UEdNode_HoudiniBuildSequenceNode::AllocateDefaultPins()
{
	if (Pins.Num() > 0)
	{
		UE_LOG(LogEHEEditor, Error, TEXT("Default pins have already been allocated."));
		return;
	}
	
	CreatePin(EGPD_Input, UHBSG_EditorTypes::PinCategory_MultipleNodes, FName(), TEXT("In"));
	CreatePin(EGPD_Output, UHBSG_EditorTypes::PinCategory_MultipleNodes, FName(), TEXT("Out"));
}

FText UEdNode_HoudiniBuildSequenceNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (!SequenceNode)
	{
		return Super::GetNodeTitle(TitleType);
	}
	if (!SequenceNode->Title.IsEmpty())
	{
		return SequenceNode->Title;
	}

	return FText::FromString("Unknown");
}

void UEdNode_HoudiniBuildSequenceNode::PrepareForCopying()
{
	Super::PrepareForCopying();
	SequenceNode->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
}

void UEdNode_HoudiniBuildSequenceNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	Super::AutowireNewNode(FromPin);

	if (FromPin != nullptr)
	{
		if (GetSchema()->TryCreateConnection(FromPin, GetInputPin()))
		{
			FromPin->GetOwningNode()->NodeConnectionListChanged();
		}
	}
}

FLinearColor UEdNode_HoudiniBuildSequenceNode::GetBackgroundColor() const
{
	return FLinearColor::Black;
}

UEdGraphPin* UEdNode_HoudiniBuildSequenceNode::GetInputPin() const
{
	return Pins[0];
}

UEdGraphPin* UEdNode_HoudiniBuildSequenceNode::GetOutputPin() const
{
	return Pins[1];
}

const FSlateBrush* UEdNode_HoudiniBuildSequenceNode::GetNodeIcon()
{
	if (SequenceNode.IsA(UHoudiniBuildSequenceNode::StaticClass()))
	{
		return FHoudiniBuildSequenceGraphEditorStyle::Get()->GetBrush(TEXT("HoudiniBuildSequenceGraphEditor.HoudiniLogo16"));
	}
	
	return FHoudiniBuildSequenceGraphEditorStyle::Get()->GetBrush(TEXT("HoudiniBuildSequenceGraphEditor.WrenchIcon"));

	// Behavior tree icon
	// return FAppStyle::GetBrush(TEXT("BTEditor.Graph.BTNode.Icon"));
}

#undef LOCTEXT_NAMESPACE