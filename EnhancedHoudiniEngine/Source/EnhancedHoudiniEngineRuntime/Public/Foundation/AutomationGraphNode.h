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

#include "AutomationGraphNode.generated.h"

UENUM()
enum class EAutomationGraphNodeState: uint8
{
	Uninitialized,
	Standby,
	Active,
	Finished,
	Expired,
	Error
};

// TODO(): Consider moving this to a separate plugin.
UCLASS()
class ENHANCEDHOUDINIENGINERUNTIME_API UAutomationGraphNode : public UObject
{
	GENERATED_BODY()

public:
	virtual bool CanActivate();
	virtual bool Activate();
	virtual void Ready();
	virtual void Reset();

	virtual void SetState(EAutomationGraphNodeState NodeState);
	virtual EAutomationGraphNodeState GetState() { return NodeState; }
	virtual FLinearColor GetStateColor();

	// Text to push out to the UI.
	virtual FString GetMessageText();

	UPROPERTY()
	TArray<TObjectPtr<UAutomationGraphNode>> ParentNodes;

	UPROPERTY()
	TArray<TObjectPtr<UAutomationGraphNode>> ChildNodes;

	// This is the text that will appear in the edit field when you first create a node. For the text that appears in
	// the node creation menu, use the DisplayName meta tag.
	//
	// IMPORTANT: This must be a UPROPERTY or else it will not be saved when you exit the editor.
	UPROPERTY()
	FText Title;

protected:
	double TimeStarted = 0.0;
	double TimeFinished = 0.0;

private:
	EAutomationGraphNodeState NodeState = EAutomationGraphNodeState::Uninitialized;
};