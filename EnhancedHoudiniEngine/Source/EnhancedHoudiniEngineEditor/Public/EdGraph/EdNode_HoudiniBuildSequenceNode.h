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
#include "Foundation/AutomationGraphNode.h"

#include "EdNode_HoudiniBuildSequenceNode.generated.h"

class UEdNode_HoudiniBuildSequenceNode;

class SEdNode_HoudiniBuildSequenceNode : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SEdNode_HoudiniBuildSequenceNode) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdNode_HoudiniBuildSequenceNode* InNode);

	//~SGraphNode interface
	virtual void UpdateGraphNode() override;
	virtual void CreatePinWidgets() override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	//~End SGraphNode interface
	
	// SNodePanel::SNode interface
	virtual void GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const override;
	// End of SNodePanel::SNode interface

	FSlateColor GetBorderBackgroundColor() const;
	const FSlateBrush* GetNameIcon() const;

	void OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo);
};
UCLASS()
class ENHANCEDHOUDINIENGINEEDITOR_API UEdNode_HoudiniBuildSequenceNode : public UEdGraphNode
{
	GENERATED_BODY()

public:
	UEdNode_HoudiniBuildSequenceNode(const FObjectInitializer& Initializer);

	UPROPERTY(VisibleAnywhere, Instanced, Category = "Houdini BSG")
	TObjectPtr<UAutomationGraphNode> SequenceNode;

	//~UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void PrepareForCopying() override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
	//~End UEdGraphNode interface

	// TODO(): determine if any of these are necessary. See SoundCueGraphNode.h. Specifically look at PostEditImport() which updates the SoundCue Owner
	// UObject interface
	/// virtual void PostLoad() override;
	/// virtual void PostEditImport() override;
	/// virtual void PostDuplicate(bool bDuplicateForPIE) override;
	// End of UObject interface

	virtual FLinearColor GetBackgroundColor() const;
	virtual UEdGraphPin* GetInputPin() const;
	virtual UEdGraphPin* GetOutputPin() const;

	virtual const FSlateBrush* GetNodeIcon();
};
