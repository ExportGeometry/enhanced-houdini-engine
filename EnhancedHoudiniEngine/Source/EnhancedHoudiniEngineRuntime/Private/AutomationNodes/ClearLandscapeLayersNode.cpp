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

#include "AutomationNodes/ClearLandscapeLayersNode.h"

#include "EHERuntimeLoggingDefs.h"
#include "EngineUtils.h"
#include "Landscape.h"

UAGN_ClearLandscapeLayers::UAGN_ClearLandscapeLayers(const FObjectInitializer& Initializer): Super(Initializer)
{
	Title = FText::FromString("ClearLandscapeLayers");
}

bool UAGN_ClearLandscapeLayers::Activate()
{
	if (!TargetLandscape.IsValid())
	{
		SetState(EAutomationGraphNodeState::Error);
		return false;
	}
	if (EditLayers.IsEmpty() || PaintLayers.IsEmpty())
	{
		SetState(EAutomationGraphNodeState::Error);
		return false;
	}

	SetState(EAutomationGraphNodeState::Active);

	ALandscape* LandscapePtr = TargetLandscape.Get();
	TMap<FName, ULandscapeLayerInfoObject*> LayerInfoByName;

	ULandscapeInfo* LandscapeInfo = LandscapePtr->GetLandscapeInfo();
	if (!LandscapeInfo)
	{
		UE_LOG(LogEHERuntime, Error, TEXT("error: UAGN_ClearLandscapeLayers::Activate(): LandscapeInfo is invalid."));
		SetState(EAutomationGraphNodeState::Error);
		return false;
	}
	for (FName PaintLayerName : PaintLayers)
	{
		ULandscapeLayerInfoObject* LayerInfo = LandscapeInfo->GetLayerInfoByName(PaintLayerName);
		if (!LayerInfo)
		{
			UE_LOG(LogEHERuntime, Error, TEXT("error: UAGN_ClearLandscapeLayers::Activate(): unknown paint layer \"%s\""), *PaintLayerName.ToString());
			SetState(EAutomationGraphNodeState::Error);
			return false;
		}
		LayerInfoByName.Add(PaintLayerName, LayerInfo);
	}

	for (FName EditLayerName : EditLayers)
	{
		int32 EditLayerIndex = LandscapePtr->GetLayerIndex(EditLayerName);
		if (EditLayerIndex == INDEX_NONE)
		{
			UE_LOG(LogEHERuntime, Error, TEXT("error: UAGN_ClearLandscapeLayers::Activate(): unknown edit layer \"%s\""), *EditLayerName.ToString());
			SetState(EAutomationGraphNodeState::Error);
			return false;
		}

		for (FName PaintLayerName : PaintLayers)
		{
			LandscapePtr->ClearPaintLayer(EditLayerIndex, LayerInfoByName[PaintLayerName]);
		}
	}
	
	SetState(EAutomationGraphNodeState::Finished);
	return true;
}

void UAGN_ClearLandscapeLayers::Initialize(UWorld* World)
{
	if (!World)
	{
		UE_LOG(LogEHERuntime, Error, TEXT("Failed to initialize ClearLandscapeLayers node: world is invalid."));
		SetState(EAutomationGraphNodeState::Error);
		return;
	}

	for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr)
	{
		AActor* CurrentActor = *ActorItr;
		auto* LandscapeActor = Cast<ALandscape>(CurrentActor);

		if (!LandscapeActor)
		{
			continue;
		}

		// Note: For now, we just grab the first landscape we can find. Technically a world can have more than one
		//       Landscape, although this generally is not recommended.
		TargetLandscape = LandscapeActor;
		break;
	}

	if (!TargetLandscape.IsValid())
	{
		UE_LOG(LogEHERuntime, Error, TEXT("Failed to initialize ClearLandscapeLayers node: Landscape is missing."));
		SetState(EAutomationGraphNodeState::Error);
		return;
	}

	SetState(EAutomationGraphNodeState::Standby);
}
