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

#include "Engine/HoudiniBuildManagerFactory.h"

#include "Editor/HoudiniBuildManagerBlueprint.h"
#include "Foundation/HoudiniBuildManager.h"
#include "Kismet2/KismetEditorUtilities.h"

UHoudiniBuildManagerBlueprintFactory::UHoudiniBuildManagerBlueprintFactory(const FObjectInitializer& Initializer): Super(Initializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UHoudiniBuildManagerBlueprint::StaticClass();
	ParentClass = AHoudiniBuildManager::StaticClass();
}

UObject* UHoudiniBuildManagerBlueprintFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	// Make sure we are trying to factory a gameplay ability blueprint, then create and init one
	check(Class->IsChildOf(UHoudiniBuildManagerBlueprint::StaticClass()));

	// If they selected an interface, force the parent class to be UInterface
	if (BlueprintType == BPTYPE_Interface)
	{
		ParentClass = UInterface::StaticClass();
	}

	if ((ParentClass == nullptr) || !FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentClass) || !ParentClass->IsChildOf(AHoudiniBuildManager::StaticClass()))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ClassName"), (ParentClass != nullptr) ? FText::FromString(ParentClass->GetName()) : NSLOCTEXT("UHoudiniBuildManagerBlueprintFactory", "Null", "(null)"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(NSLOCTEXT("UHoudiniBuildManagerBlueprintFactory", "CannotCreateHoudiniBuildManagerBlueprint", "Cannot create a HoudiniBuildManagerBlueprint based on the class '{ClassName}'."), Args));
		return nullptr;
	}
	else
	{
		return CastChecked<UHoudiniBuildManagerBlueprint>(FKismetEditorUtilities::CreateBlueprint(ParentClass, InParent, Name, BlueprintType, UHoudiniBuildManagerBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), CallingContext));
	}
}

UObject* UHoudiniBuildManagerBlueprintFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return FactoryCreateNew(Class, InParent, Name, Flags, Context, Warn, NAME_None);
}

UHoudiniBuildManagerFactory::UHoudiniBuildManagerFactory(const FObjectInitializer& Initializer): Super(Initializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = AHoudiniBuildManager::StaticClass();
}

UObject* UHoudiniBuildManagerFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<AHoudiniBuildManager>(InParent, Class, Name, Flags | RF_Transactional);
}
