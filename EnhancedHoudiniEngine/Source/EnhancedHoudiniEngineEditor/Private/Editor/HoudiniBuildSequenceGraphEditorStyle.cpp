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

#include "Editor/HoudiniBuildSequenceGraphEditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Misc/Paths.h"

TSharedPtr<FSlateStyleSet> FHoudiniBuildSequenceGraphEditorStyle::StyleSet = nullptr;

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define ENGINE_IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( (FPaths::EngineContentDir() / RelativePath) + TEXT(".png"), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( StyleSet->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( StyleSet->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

void FHoudiniBuildSequenceGraphEditorStyle::Initialize()
{
	const FVector2D Icon16x16(24.0f, 24.0f);
	const FVector2D Icon20x20(20.0f, 20.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);
	const FVector2D Icon64x64(64.0f, 64.0f);

	if (StyleSet.IsValid())
	{
		return;
	}

	StyleSet = MakeShareable(new FSlateStyleSet("HoudiniBuildSequnceGraphEditorStyle"));
	
	StyleSet->SetContentRoot(FPaths::ProjectPluginsDir() / TEXT("EnhancedHoudiniEngine/Resources"));
	
	StyleSet->Set("HoudiniBuildSequenceGraphEditor.HoudiniLogo16", new IMAGE_BRUSH("Icons/icon_houdini_logo_24", Icon16x16));
	StyleSet->Set("HoudiniBuildSequenceGraphEditor.WrenchIcon", new ENGINE_IMAGE_BRUSH("Editor/Slate/Icons/wrench_16x", Icon16x16));

	StyleSet->Set("HoudiniBuildSequenceGraphEditor.NodeBorder.Invisible", new FSlateBoxBrush(FName(NAME_None), FMargin(0), FLinearColor(1.0f, 1.0f, 1.0f, 0.0)));
	StyleSet->Set("HoudiniBuildSequenceGraphEditor.NodeBorder.Default", new FSlateBoxBrush(FName(NAME_None), FMargin(0), FLinearColor(1.0f, 1.0f, 1.0f, 0.0)));
	StyleSet->Set("HoudiniBuildSequenceGraphEditor.NodeBorder.DefaultHovered", new FSlateBoxBrush(FName(NAME_None), FMargin(0), FLinearColor::White));

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

void FHoudiniBuildSequenceGraphEditorStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}

TSharedPtr<ISlateStyle> FHoudiniBuildSequenceGraphEditorStyle::Get()
{
	return StyleSet;	
}

const FName& FHoudiniBuildSequenceGraphEditorStyle::GetStyleSetName()
{
	return StyleSet->GetStyleSetName();
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT
