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
#include "AssetTypeActions/AssetTypeActions_Blueprint.h"
#include "Factories/Factory.h"

// TODO(): This FAssetTypeActions_Blueprint is deprecated in favor of UAssetDefinition_Blueprint. HOWEVER, UAssetDefinition_Blueprint
//         is not actually exported with a _API tag as of UE 5.4. Not sure if this is intentional or an oversight from epic.
class FAssetTypeActions_HoudiniBuildManagerBlueprint : public FAssetTypeActions_Blueprint
{
public:
	FAssetTypeActions_HoudiniBuildManagerBlueprint(EAssetTypeCategories::Type NewAssetCategory);
	
	//~IAssetTypeActions interface
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override { return FColor(255, 102, 0); }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override { return AssetCategory; }
	//~End IAssetTypeActions interface

	//~FAssetTypeActions_Blueprint interface
	void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor) override;
	//~End FAssetTypeActions_Bluerint interface

protected:
	//~FAssetTypeActions_Blueprint interface
	virtual UFactory* GetFactoryForBlueprintType(UBlueprint* InBlueprint) const override;
	//~End FAssetTypeActions_Blueprint interface
	
	EAssetTypeCategories::Type AssetCategory;

private:
	/** Returns true if the blueprint is data only */
	bool ShouldUseDataOnlyEditor( const UBlueprint* Blueprint ) const;
};