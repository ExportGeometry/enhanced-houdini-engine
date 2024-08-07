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

#include "Foundation/AutomationGraphNode.h"

bool UAutomationGraphNode::CanActivate()
{
	if (NodeState == EAutomationGraphNodeState::Uninitialized || NodeState == EAutomationGraphNodeState::Active)
	{
		return false;
	}
	
	if (ParentNodes.IsEmpty())
	{
		return true;
	}

	// Note: This is a bit inefficient since we will most likely be calling CanActivate() on multiple children pointing
	//       to the same parent. At some point it might be worth setting up some kind of caching for the parent State.
	for (TObjectPtr<UAutomationGraphNode> ParentNode : ParentNodes)
	{
		if (ParentNode->GetState() != EAutomationGraphNodeState::Finished)
		{
			return false;
		}
	}

	return true;
}

bool UAutomationGraphNode::Activate()
{
	if (NodeState != EAutomationGraphNodeState::Standby)
	{
		return false;
	}

	SetState(EAutomationGraphNodeState::Finished);
	return true;
}

void UAutomationGraphNode::Ready()
{
	// By defualt, we don't allow a node to ready if it is actively doing something.
	if (NodeState == EAutomationGraphNodeState::Active)
	{
		return;
	}
	
	SetState(EAutomationGraphNodeState::Standby);
}

void UAutomationGraphNode::Reset()
{
	SetState(EAutomationGraphNodeState::Uninitialized);
}

void UAutomationGraphNode::SetState(EAutomationGraphNodeState NewState)
{
	NodeState = NewState;

	switch (NodeState)
	{
	case EAutomationGraphNodeState::Uninitialized:
	case EAutomationGraphNodeState::Standby:
		TimeStarted = 0.0;
		TimeFinished = 0.0;
		break;
	case EAutomationGraphNodeState::Active:
		TimeStarted = FPlatformTime::Seconds();
		break;
	case EAutomationGraphNodeState::Finished:
	case EAutomationGraphNodeState::Error:
	case EAutomationGraphNodeState::Expired:
		TimeFinished = FPlatformTime::Seconds();
		break;
	default:
		break;
	}
}

FLinearColor UAutomationGraphNode::GetStateColor()
{
	switch(NodeState)
	{
	case EAutomationGraphNodeState::Uninitialized:
	case EAutomationGraphNodeState::Standby:
		return FLinearColor(0.08f, 0.08f, 0.08f);
	case EAutomationGraphNodeState::Active:
		return FLinearColor::Yellow;
	case EAutomationGraphNodeState::Finished:
		return FLinearColor::Green;
	case EAutomationGraphNodeState::Error:
		return FLinearColor::Red;
	case EAutomationGraphNodeState::Expired:
		return FLinearColor::Gray;
	default:
		return FLinearColor(1.0, 0.0, 1.0);		
	}
}

FString UAutomationGraphNode::GetMessageText()
{
	double ActiveTime = 0.0;
	double TotalTime = 0.0;
	
	switch (NodeState)
	{
	case EAutomationGraphNodeState::Active:
		ActiveTime = FPlatformTime::Seconds() - TimeStarted;
		return FString::Printf(TEXT("Active for %.2lf Seconds"), ActiveTime); 
	case EAutomationGraphNodeState::Finished:
		TotalTime = TimeFinished - TimeStarted;
		return FString::Printf(TEXT("Finished in %.2lf Seconds"), TotalTime); 
	case EAutomationGraphNodeState::Expired:
		return FString("Expired.");
	default:
		break;
	}

	return FString();
}
