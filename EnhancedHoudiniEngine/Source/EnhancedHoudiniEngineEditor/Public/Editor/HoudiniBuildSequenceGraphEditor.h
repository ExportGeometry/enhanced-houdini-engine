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
#include "CoreMinimal.h"

class AHoudiniBuildManager;
class FHoudiniBuildSequenceGraphEditor;
class UHoudiniBuildSequenceGraph;

class FHoudiniBSGEditorCommands : public TCommands<FHoudiniBSGEditorCommands>
{
public:
	FHoudiniBSGEditorCommands(): TCommands<FHoudiniBSGEditorCommands>(
		TEXT("HoudiniBuildSequenceGraphEditor"),
		NSLOCTEXT("Contexts", "TileMapEditor", "Tile Map Editor"),
		NAME_None,
		FAppStyle::GetAppStyleSetName()
		)
	{
	}

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> ExecuteGraph;
};

struct FHoudiniBSGManagerSelectionObject
{
public:
	FHoudiniBSGManagerSelectionObject(TWeakObjectPtr<AHoudiniBuildManager> InPtr, const FString& InLabel);
	
	/** Returns true if this is the special entry for no specific object */
	bool IsEmptyObject() const
	{
		return ObjectPath.IsEmpty();
	}

	/** If this has no editor path, it was spawned */
	bool IsSpawnedObject() const 
	{
		return !ObjectPath.IsEmpty() && EditorObjectPath.IsEmpty();
	}
	
	/** If editor and object path are the same length because there's no prefix, this is the editor object */
	bool IsEditorObject() const 
	{
		return !ObjectPath.IsEmpty() && ObjectPath.Len() == EditorObjectPath.Len();
	}
	
	/** Actual object to debug, can be null */
	TWeakObjectPtr<AHoudiniBuildManager> BuildManager;

	/** Friendly label for object to debug */
	FString ObjectLabel;

	/** Raw object path of spawned PIE object, this is not a SoftObjectPath because we don't want it to get fixed up */
	FString ObjectPath;

	/** Object path to object in the editor, will only be set for static objects */
	FString EditorObjectPath;
};

class SHoudiniBSGManagerSelectorWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SHoudiniBSGManagerSelectorWidget){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<FHoudiniBuildSequenceGraphEditor> InGraphEditor);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

protected:
	TSharedRef<SWidget> OnGetActiveDetailSlotContent(bool bChangedToHighDetail);
	
	TSharedPtr<FHoudiniBSGManagerSelectionObject> GetCurrentlySelectedObject() const;
	void OnSelectionBoxOpening();
	void GenerateSelectionObjects();
	void SelectionBoxSelectionChanged(TSharedPtr<FHoudiniBSGManagerSelectionObject> NewSelection, ESelectInfo::Type SelectInfo);
	TSharedRef<SWidget> CreateSelectionListItem(TSharedPtr<FHoudiniBSGManagerSelectionObject> InItem);
	FText GetSelectedDebugObjectTextLabel() const;

	UWorld* GetEditorWorld();
	FString MakeSelectionObjectLabel(UObject* SelectionObject, bool bAddContextIfSelectedInEditor, bool bAddSpawnedContext) const;
	
	TWeakPtr<FHoudiniBuildSequenceGraphEditor> GraphEditor;

	TArray<TSharedPtr<FHoudiniBSGManagerSelectionObject>> SelectionObjects;
	TSharedPtr<SComboBox<TSharedPtr<FHoudiniBSGManagerSelectionObject>>> SelectionBox;

	TWeakObjectPtr<AHoudiniBuildManager> LastManagerSelected;
};

class FHoudiniBuildSequenceGraphEditor : public FAssetEditorToolkit, public FNotifyHook, public FGCObject
{
public:
	FHoudiniBuildSequenceGraphEditor();
	virtual ~FHoudiniBuildSequenceGraphEditor();
	
	void Initialize(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UHoudiniBuildSequenceGraph* NewSequenceGraph);
	void BuildCustomCommands();
	void BuildGraphEditorCommands();
	void CreateInternalWidgets();
	void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection);

	FGraphPanelSelectionSet GetSelectedNodes() const;
	
	//~IToolkit interface
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	//~End IToolkit interface
	
	//~FAssetEditorToolkit interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FText GetToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FString GetDocumentationLink() const override;
	virtual void OnClose() override;
	/// virtual void SaveAsset_Execute() override;
	//~End FAssetEditorToolkit interface

	//~FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override
	{
		return TEXT("FHoudiniBuildSequenceGraphEditor");
	}
	//~End FGCObject interface

// Representative Object (the object this editor is editing)
public:
	TObjectPtr<UHoudiniBuildSequenceGraph> SequenceGraph;
	
// Editor Commands
protected:
	bool CanSelectAllNodes();
	void SelectAllNodes();

	bool CanDeleteNodes();
	void DeleteSelectedNodes();
	void DeleteSelectedDuplicatableNodes();
	
	bool CanCutNodes();
	void CutSelectedNodes();

	bool CanCopyNodes();
	void CopySelectedNodes();

	bool CanPasteNodes();
	void PasteNodes();
	void PasteNodesHere(const FVector2D& Location);

	bool CanDuplicateNodes();
	void DuplicateNodes();

	bool CanRenameNodes() const;
	void OnRenameNode();

	TSharedPtr<FUICommandList> GraphEditorCommands;

// Main Viewport
protected:
	TSharedRef<SDockTab> SpawnTab_GraphCanvas(const FSpawnTabArgs& Args);
	
	TSharedPtr<SGraphEditor> SlateGraphEditor;
	static const FName GraphCanvasTabId;

// properties panel
protected:
	TSharedRef<SDockTab> SpawnTab_Properties(const FSpawnTabArgs& Args);

	TSharedPtr<class IDetailsView> NodeProperties;
	static const FName PropertiesTabId;

// Toolbar
public:
	TWeakObjectPtr<AHoudiniBuildManager> GetSelectedManager() { return SelectedManager; }
	void SetSelectedManager(TWeakObjectPtr<AHoudiniBuildManager> NewSelectedManager);
	
protected:
	void ExtendToolbar();

	bool CanExecuteGraph() const;
	void ExecuteGraph();
	
	TWeakObjectPtr<AHoudiniBuildManager> SelectedManager;
};
