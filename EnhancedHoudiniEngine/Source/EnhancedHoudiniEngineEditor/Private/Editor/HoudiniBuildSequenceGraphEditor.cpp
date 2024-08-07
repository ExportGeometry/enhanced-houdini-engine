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

#include "Editor/HoudiniBuildSequenceGraphEditor.h"

#include "EdGraphUtilities.h"
#include "EHEEditorLoggingDefs.h"
#include "EdGraph/EdGraph_HoudiniBuildSequenceGraph.h"
#include "EdGraph/EdNode_HoudiniBuildSequenceEdge.h"
#include "EdGraph/EdNode_HoudiniBuildSequenceNode.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Foundation/HoudiniBuildManager.h"
#include "Foundation/HoudiniBuildSequenceGraph.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphEditorActions.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "SLevelOfDetailBranchNode.h"

#define LOCTEXT_NAMESPACE "HoudiniBuildSequenceGraphEditor"

const FName FHoudiniBuildSequenceGraphEditor::GraphCanvasTabId( TEXT( "HBSGEditor_GraphCanvas" ) );
const FName FHoudiniBuildSequenceGraphEditor::PropertiesTabId( TEXT( "HBSGEditor_PropertiesTab" ) );

void FHoudiniBSGEditorCommands::RegisterCommands()
{
	UI_COMMAND(ExecuteGraph, "Run", "Run", EUserInterfaceActionType::Button, FInputChord());
}

FHoudiniBSGManagerSelectionObject::FHoudiniBSGManagerSelectionObject(TWeakObjectPtr<AHoudiniBuildManager> InPtr, const FString& InLabel): BuildManager(InPtr), ObjectLabel(InLabel)
{
	if (BuildManager.IsValid())
	{
		ObjectPath = BuildManager->GetPathName();

		// Compute non-PIE path
		FString OriginalPath = UWorld::RemovePIEPrefix(ObjectPath);

		// Look for original object
		UObject* OriginalObject = FindObjectSafe<UObject>(nullptr, *OriginalPath);

		if (OriginalObject)
		{
			EditorObjectPath = OriginalPath;
		}
		else
		{
			// No editor path, was dynamically spawned
			EditorObjectPath = FString();
		}
	}
	else
	{
		ObjectPath = EditorObjectPath = FString();
	}
}

void SHoudiniBSGManagerSelectorWidget::Construct(const FArguments& InArgs, TWeakPtr<FHoudiniBuildSequenceGraphEditor> InGraphEditor)
{
	GraphEditor = InGraphEditor;
	LastManagerSelected = nullptr;

	GenerateSelectionObjects();
	
	SelectionBox = SNew(SComboBox<TSharedPtr<FHoudiniBSGManagerSelectionObject>>)
	.ToolTipText(LOCTEXT("SelectionBoxTooltip", "Select a HoudiniBuildManager linked to this graph"))
	.OptionsSource(&SelectionObjects)
	.InitiallySelectedItem(GetCurrentlySelectedObject())
	.OnComboBoxOpening(this, &SHoudiniBSGManagerSelectorWidget::OnSelectionBoxOpening)
	.OnSelectionChanged(this, &SHoudiniBSGManagerSelectorWidget::SelectionBoxSelectionChanged)
	.OnGenerateWidget(this, &SHoudiniBSGManagerSelectorWidget::CreateSelectionListItem)
	.ContentPadding(FMargin(0.f, 4.f))
	.AddMetaData<FTagMetaData>(TEXT("SelectedObjectLabel"))
	[
		SNew(STextBlock)
		.Text(this, &SHoudiniBSGManagerSelectorWidget::GetSelectedDebugObjectTextLabel)
	];

	ChildSlot
	[
		SNew(SLevelOfDetailBranchNode)
		.UseLowDetailSlot(FMultiBoxSettings::UseSmallToolBarIcons)
		.OnGetActiveDetailSlotContent(this, &SHoudiniBSGManagerSelectorWidget::OnGetActiveDetailSlotContent)
	];
}

void SHoudiniBSGManagerSelectorWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (!GraphEditor.IsValid())
	{
		return;
	}

	TWeakObjectPtr<AHoudiniBuildManager> SelectedManager = GraphEditor.Pin()->GetSelectedManager();
	
	if (SelectedManager.IsValid())
	{
		// TODO(): Is this actually needed?
		if (SelectedManager != LastManagerSelected)
		{
			GenerateSelectionObjects();
			LastManagerSelected = SelectedManager;
		}
	}
	else
	{
		// Only refresh if we aren't currently pointed at the "empty" menu item.
		TSharedPtr<FHoudiniBSGManagerSelectionObject> CurrentSelection = SelectionBox->GetSelectedItem();
		if (CurrentSelection.IsValid() && CurrentSelection->IsEditorObject())
		{
			LastManagerSelected = nullptr;
			GenerateSelectionObjects(); // will reset to the first ("nothing selected...") value, since SelectedManager is not valid
		}
	}
}

TSharedRef<SWidget> SHoudiniBSGManagerSelectorWidget::OnGetActiveDetailSlotContent(bool bChangedToHighDetail)
{
	TSharedRef<SWidget> BrowseWidget =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin(8.0f, 0.0f, 0.0f, 0.0f))
		[
			SelectionBox.ToSharedRef()
		];
	
	return
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.Padding(0.0f)
		.AutoWidth()
		[
			BrowseWidget
		];
}

TSharedPtr<FHoudiniBSGManagerSelectionObject> SHoudiniBSGManagerSelectorWidget::GetCurrentlySelectedObject() const
{
	if (GraphEditor.IsValid())
	{
		TWeakObjectPtr<AHoudiniBuildManager> SelectedManager = GraphEditor.Pin()->GetSelectedManager();
		if (SelectedManager.IsValid())
		{
			for (TSharedPtr<FHoudiniBSGManagerSelectionObject> SelectionObject : SelectionObjects)
			{
				if (
					SelectionObject.IsValid() &&
					SelectionObject->BuildManager.IsValid() &&
					SelectionObject->ObjectPath == SelectedManager->GetPathName() // TODO(): Make sure that PathName is valid outside of PIE
				)
				{
					return SelectionObject;	
				}
			}
		}
	}
	
	if (SelectionObjects.Num() > 0)
	{
		return SelectionObjects[0];
	}

	return nullptr;
}

void SHoudiniBSGManagerSelectorWidget::OnSelectionBoxOpening()
{
	GenerateSelectionObjects();
}

void SHoudiniBSGManagerSelectorWidget::GenerateSelectionObjects()
{
	SelectionObjects.Empty();
	SelectionObjects.Add(
		MakeShareable(new FHoudiniBSGManagerSelectionObject(
			nullptr,
			LOCTEXT("NoManagerSelected", "No Manager Selected").ToString()
		))
	);
	
	UWorld* EditorWorld = GetEditorWorld();
	if (!EditorWorld)
	{
		return;
	}
	if (!GraphEditor.IsValid() || !GraphEditor.Pin()->SequenceGraph)
	{
		return;
	}

	// Find each HoudiniBuildManager that is linked to this editor's sequence graph.
	for (TActorIterator<AActor> ActorItr(EditorWorld); ActorItr; ++ActorItr)
	{
		AActor* CurrentActor = *ActorItr;
		auto ManagerActor = Cast<AHoudiniBuildManager>(CurrentActor);

		if (!ManagerActor)
		{
			continue;
		}

		if (ManagerActor->SequenceGraph == GraphEditor.Pin()->SequenceGraph)
		{
			TSharedPtr<FHoudiniBSGManagerSelectionObject> NewInstance = MakeShareable(new FHoudiniBSGManagerSelectionObject(ManagerActor, FString()));
			NewInstance->ObjectLabel = MakeSelectionObjectLabel(ManagerActor, true, NewInstance->IsSpawnedObject());
			
			SelectionObjects.Add(NewInstance);
		}
	}

	if (SelectionBox.IsValid())
	{
		SelectionBox->SetSelectedItem(GetCurrentlySelectedObject());
		SelectionBox->RefreshOptions();
	}
}

void SHoudiniBSGManagerSelectorWidget::SelectionBoxSelectionChanged(TSharedPtr<FHoudiniBSGManagerSelectionObject> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (NewSelection != GetCurrentlySelectedObject() && NewSelection.IsValid())
	{
		if (GraphEditor.IsValid())
		{
			TWeakObjectPtr<AHoudiniBuildManager> SelectedManager = NewSelection->BuildManager;
			GraphEditor.Pin()->SetSelectedManager(SelectedManager);
			LastManagerSelected = SelectedManager;
		}
	}
	else if (!NewSelection.IsValid())
	{
		if (GraphEditor.IsValid() && GraphEditor.Pin()->GetSelectedManager().IsValid())
		{
			GraphEditor.Pin()->SetSelectedManager(nullptr);
			LastManagerSelected = nullptr;
		}
	}
}

TSharedRef<SWidget> SHoudiniBSGManagerSelectorWidget::CreateSelectionListItem(TSharedPtr<FHoudiniBSGManagerSelectionObject> InItem)
{
	FString ItemString;
	FString ItemTooltip;

	if (InItem.IsValid())
	{
		ItemString = InItem->ObjectLabel;
		ItemTooltip = InItem->ObjectPath;
	}

	return SNew(STextBlock)
		.Text(FText::FromString(*ItemString))
		.ToolTipText(FText::FromString(*ItemTooltip));
}

FText SHoudiniBSGManagerSelectorWidget::GetSelectedDebugObjectTextLabel() const
{
	FString Label;
	TSharedPtr<FHoudiniBSGManagerSelectionObject> SelectionObject = GetCurrentlySelectedObject();
	if (SelectionObject.IsValid())
	{
		Label = SelectionObject->ObjectLabel;
	}

	return FText::FromString(Label);
}

UWorld* SHoudiniBSGManagerSelectorWidget::GetEditorWorld()
{
	UWorld* World = nullptr;
	
#if WITH_EDITOR
	auto* EditorEngine = Cast<UEditorEngine>(GEngine);
	if (GIsEditor && EditorEngine != nullptr)
	{
		World = EditorEngine->GetEditorWorldContext().World();
	}
#endif
	return World;	
}

FString SHoudiniBSGManagerSelectorWidget::MakeSelectionObjectLabel(UObject* TestObject, bool bAddContextIfSelectedInEditor, bool bAddSpawnedContext) const
{
	auto GetActorLabelStringLambda = [](AActor* InActor, bool bIncludeNetModeSuffix, bool bIncludeSelectedSuffix, bool bIncludeSpawnedContext)
	{
		FString Label = InActor->GetActorLabel();

		FString Context;

		if (bIncludeNetModeSuffix)
		{
			switch (InActor->GetNetMode())
			{
			case ENetMode::NM_Client:
			{
				Context = LOCTEXT("DebugWorldClient", "Client").ToString();

				FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(InActor->GetWorld());
				if (WorldContext != nullptr && WorldContext->PIEInstance > 1)
				{
					Context += TEXT(" ");
					Context += FText::AsNumber(WorldContext->PIEInstance - 1).ToString();
				}
			}
			break;

			case ENetMode::NM_ListenServer:
			case ENetMode::NM_DedicatedServer:
				Context = LOCTEXT("DebugWorldServer", "Server").ToString();
				break;
			}
		}

		if (bIncludeSpawnedContext)
		{
			if (!Context.IsEmpty())
			{
				Context += TEXT(", ");
			}

			Context += LOCTEXT("DebugObjectSpawned", "spawned").ToString();
		}

		if (bIncludeSelectedSuffix && InActor->IsSelected())
		{
			if (!Context.IsEmpty())
			{
				Context += TEXT(", ");
			}

			Context += LOCTEXT("DebugObjectSelected", "selected").ToString();
		}

		if (!Context.IsEmpty())
		{
			Label = FString::Printf(TEXT("%s (%s)"), *Label, *Context);
		}

		return Label;
	};
	
	const bool bIncludeNetModeSuffix = false;

	FString Label;
	if (AActor* Actor = Cast<AActor>(TestObject))
	{
		Label = GetActorLabelStringLambda(Actor, bIncludeNetModeSuffix, bAddContextIfSelectedInEditor, bAddSpawnedContext);
	}
	else
	{
		if (AActor* ParentActor = TestObject->GetTypedOuter<AActor>())
		{
			// We don't need the full path because it's in the tooltip
			const FString RelativePath = TestObject->GetName();
			Label = FString::Printf(TEXT("%s in %s"), *RelativePath, *GetActorLabelStringLambda(ParentActor, bIncludeNetModeSuffix, bAddContextIfSelectedInEditor, bAddSpawnedContext));
		}
		else
		{
			Label = TestObject->GetName();
		}
	}

	return Label;
}

FHoudiniBuildSequenceGraphEditor::FHoudiniBuildSequenceGraphEditor()
{
	SequenceGraph = nullptr;
}

FHoudiniBuildSequenceGraphEditor::~FHoudiniBuildSequenceGraphEditor()
{
	
}

void FHoudiniBuildSequenceGraphEditor::Initialize(
	const EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost>& InitToolkitHost,
	UHoudiniBuildSequenceGraph* NewSequenceGraph
)
{
	SequenceGraph = NewSequenceGraph;

	FHoudiniBSGEditorCommands::Register();

	// Create the corresponding UEdGraph
	if (SequenceGraph->EditorGraph == nullptr && !SequenceGraph->HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
	{
		SequenceGraph->EditorGraph = CastChecked<UEdGraph_HoudiniBuildSequenceGraph>(
		FBlueprintEditorUtils::CreateNewGraph(
				SequenceGraph,
				NAME_None,
				UEdGraph_HoudiniBuildSequenceGraph::StaticClass(),
				UEdGraphSchema_HoudiniBuildSequenceGraph::StaticClass()
			)
		);
		SequenceGraph->EditorGraph->bAllowDeletion = false;

		// Give the schema a chance to fill out any required nodes.
		const UEdGraphSchema* Schema = SequenceGraph->EditorGraph->GetSchema();
		Schema->CreateDefaultNodesForGraph(*SequenceGraph->EditorGraph);
	}

	FGenericCommands::Register();
	FGraphEditorCommands::Register();

	BuildCustomCommands();
	CreateInternalWidgets();

	// IMPORTANT: Unreal silently caches this layout to an ini file. This means that if you make ANY change to this
	//            layout, you must increment the LayoutName suffix (Layout_v[NUM++]).
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_HoudiniBuildSequenceGraphEditor_Layout_v10")
	->AddArea
	(
		FTabManager::NewPrimaryArea()
		->SetOrientation(Orient_Vertical)
		->Split(FTabManager::NewSplitter()
			->SetOrientation(Orient_Horizontal)
			->SetSizeCoefficient(0.9f)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.75f)
				->SetHideTabWell(true)
				->AddTab(GraphCanvasTabId, ETabState::OpenedTab)
			)
			->Split(
			FTabManager::NewStack()
				->SetSizeCoefficient(0.25f)
				->SetHideTabWell(true)
				->AddTab(PropertiesTabId, ETabState::OpenedTab)
			)
		)
	);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(
		Mode,
		InitToolkitHost,
		TEXT("HoudiniBuildSequenceGraphEditorApp"),
		StandaloneDefaultLayout,
		bCreateDefaultStandaloneMenu,
		bCreateDefaultToolbar,
		NewSequenceGraph);

	ExtendToolbar();
	RegenerateMenusAndToolbars();
}

void FHoudiniBuildSequenceGraphEditor::BuildCustomCommands()
{
	ToolkitCommands->MapAction(
		FHoudiniBSGEditorCommands::Get().ExecuteGraph,
		FExecuteAction::CreateSP(this, &FHoudiniBuildSequenceGraphEditor::ExecuteGraph),
		FCanExecuteAction::CreateSP(this, &FHoudiniBuildSequenceGraphEditor::CanExecuteGraph)
	);
}

void FHoudiniBuildSequenceGraphEditor::BuildGraphEditorCommands()
{
	if (GraphEditorCommands.IsValid())
	{
		// Already built, nothing to do...
		return;
	}
	
	GraphEditorCommands = MakeShareable(new FUICommandList);

	GraphEditorCommands->MapAction(FGenericCommands::Get().SelectAll,
		FExecuteAction::CreateRaw(this, &FHoudiniBuildSequenceGraphEditor::SelectAllNodes),
		FCanExecuteAction::CreateRaw(this, &FHoudiniBuildSequenceGraphEditor::CanSelectAllNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Delete,
		FExecuteAction::CreateRaw(this, &FHoudiniBuildSequenceGraphEditor::DeleteSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FHoudiniBuildSequenceGraphEditor::CanDeleteNodes)
	);
	
	GraphEditorCommands->MapAction(FGenericCommands::Get().Copy,
		FExecuteAction::CreateRaw(this, &FHoudiniBuildSequenceGraphEditor::CopySelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FHoudiniBuildSequenceGraphEditor::CanCopyNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Cut,
		FExecuteAction::CreateRaw(this, &FHoudiniBuildSequenceGraphEditor::CutSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FHoudiniBuildSequenceGraphEditor::CanCutNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Paste,
		FExecuteAction::CreateRaw(this, &FHoudiniBuildSequenceGraphEditor::PasteNodes),
		FCanExecuteAction::CreateRaw(this, &FHoudiniBuildSequenceGraphEditor::CanPasteNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateRaw(this, &FHoudiniBuildSequenceGraphEditor::DuplicateNodes),
		FCanExecuteAction::CreateRaw(this, &FHoudiniBuildSequenceGraphEditor::CanDuplicateNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &FHoudiniBuildSequenceGraphEditor::OnRenameNode),
		FCanExecuteAction::CreateSP(this, &FHoudiniBuildSequenceGraphEditor::CanRenameNodes)
	);
}

void FHoudiniBuildSequenceGraphEditor::CreateInternalWidgets()
{
	BuildGraphEditorCommands();

	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_HBSG", "Houdini Build Sequence Graph");
	
	SGraphEditor::FGraphEditorEvents InEvents;

	// TODO(): Decide if any of these are necessary
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FHoudiniBuildSequenceGraphEditor::OnSelectedNodesChanged);
	/// InEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FSoundCueEditor::OnNodeTitleCommitted);
	/// InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FSoundCueEditor::PlaySingleNode);

	SlateGraphEditor = SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(true)
		.Appearance(AppearanceInfo)
		.GraphToEdit(SequenceGraph->EditorGraph)
		.GraphEvents(InEvents)
		.AutoExpandActionMenu(true)
		.ShowGraphStateOverlay(false);

	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.NotifyHook = this;
	auto& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	NodeProperties = PropertyModule.CreateDetailView(Args);
	NodeProperties->SetObject(SequenceGraph);
}

void FHoudiniBuildSequenceGraphEditor::OnSelectedNodesChanged(const TSet<UObject*>& NewSelection)
{
	TArray<UObject*> Selection;

	for (UObject* SelectionEntry : NewSelection)
	{
		Selection.Add(SelectionEntry);
	}

	if (Selection.Num() == 0) 
	{
		NodeProperties->SetObject(SequenceGraph);

	}
	else
	{
		NodeProperties->SetObjects(Selection);
	}
}

FGraphPanelSelectionSet FHoudiniBuildSequenceGraphEditor::GetSelectedNodes() const
{
	FGraphPanelSelectionSet CurrentSelection;
	if (SlateGraphEditor.IsValid())
	{
		CurrentSelection = SlateGraphEditor->GetSelectedNodes();
	}
	return CurrentSelection;
}

void FHoudiniBuildSequenceGraphEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_HoudiniBuildSequenceGraphEditor", "Houdini BuildSequenceGraph Editor"));
	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	// Spawn the graph canvas
	InTabManager->RegisterTabSpawner( GraphCanvasTabId, FOnSpawnTab::CreateSP(this, &FHoudiniBuildSequenceGraphEditor::SpawnTab_GraphCanvas) )
		.SetDisplayName( LOCTEXT("GraphCanvasTab", "Viewport") )
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));

	// Spawn the details panel
	InTabManager->RegisterTabSpawner( PropertiesTabId, FOnSpawnTab::CreateSP(this, &FHoudiniBuildSequenceGraphEditor::SpawnTab_Properties) )
		.SetDisplayName( LOCTEXT("DetailsTab", "Details") )
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FHoudiniBuildSequenceGraphEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(GraphCanvasTabId);
	InTabManager->UnregisterTabSpawner(PropertiesTabId);
}

FName FHoudiniBuildSequenceGraphEditor::GetToolkitFName() const
{
	return FName("FHoudiniBuildSequenceGraphEditor");	
}

FText FHoudiniBuildSequenceGraphEditor::GetBaseToolkitName() const
{
	return LOCTEXT("HoudiniBuildSequenceGraphEditorAppLabel", "Houdini BuildSequenceGraph Editor");	
}

FText FHoudiniBuildSequenceGraphEditor::GetToolkitName() const
{
	// UE seems to check dirty state for us these days. Leaving this in for posterity.
	// const bool bDirtyState = SequenceGraph->GetOutermost()->IsDirty();

	FFormatNamedArguments Args;
	Args.Add(TEXT("BuildSequenceGraphName"), FText::FromString(SequenceGraph->GetName()));
	// Args.Add(TEXT("DirtyState"), bDirtyState ? FText::FromString(TEXT("*")) : FText::GetEmpty());
	return FText::Format(LOCTEXT("HoudiniBuildSequenceGraphEditorToolkitName", "{BuildSequenceGraphName}"), Args);
}

FText FHoudiniBuildSequenceGraphEditor::GetToolkitToolTipText() const
{
	return FAssetEditorToolkit::GetToolTipTextForObject(SequenceGraph);	
}

FLinearColor FHoudiniBuildSequenceGraphEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

FString FHoudiniBuildSequenceGraphEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("HoudiniBuildSequenceGraphEditor");
}

FString FHoudiniBuildSequenceGraphEditor::GetDocumentationLink() const
{
	return TEXT("");
}

void FHoudiniBuildSequenceGraphEditor::OnClose()
{
	if (SequenceGraph)
	{
		SequenceGraph->Reset();
	}
	
	FAssetEditorToolkit::OnClose();
}

void FHoudiniBuildSequenceGraphEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(SequenceGraph);	
}

bool FHoudiniBuildSequenceGraphEditor::CanSelectAllNodes()
{
	return true;
}
void FHoudiniBuildSequenceGraphEditor::SelectAllNodes()
{
	if (!SlateGraphEditor.IsValid())
	{
		return;
	}

	SlateGraphEditor->SelectAllNodes();
}

bool FHoudiniBuildSequenceGraphEditor::CanDeleteNodes()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node != nullptr && Node->CanUserDeleteNode())
		{
			return true;
		}
	}

	return false;
}
void FHoudiniBuildSequenceGraphEditor::DeleteSelectedNodes()
{
	if (!SlateGraphEditor.IsValid())
	{
		return;
	}

	const FScopedTransaction Transaction(FGenericCommands::Get().Delete->GetDescription());
	SlateGraphEditor->GetCurrentGraph()->Modify();
	const FGraphPanelSelectionSet SelectedNodes = SlateGraphEditor->GetSelectedNodes();
	SlateGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* Node = CastChecked<UEdGraphNode>(*NodeIt);

		if (Node->CanUserDeleteNode())
		{
			if (auto* EdNode = Cast<UEdNode_HoudiniBuildSequenceNode>(Node))
			{
				FBlueprintEditorUtils::RemoveNode(NULL, EdNode, true);

				// Make sure SequenceGraph is updated to match graph
				CastChecked<UEdGraph_HoudiniBuildSequenceGraph>(SequenceGraph->EditorGraph)->RebuildSequenceGraph();
				SequenceGraph->MarkPackageDirty();
			}
			else
			{
				UE_LOG(LogEHEEditor, Warning, TEXT("warning: FHoudiniBuildSequenceGraphEditor::DeleteSelectedNodes(): unknown node type"));
				FBlueprintEditorUtils::RemoveNode(NULL, Node, true);
			}
		}
	}
}

void FHoudiniBuildSequenceGraphEditor::DeleteSelectedDuplicatableNodes()
{
	if (!SlateGraphEditor.IsValid())
	{
		return;
	}

	const FGraphPanelSelectionSet OldSelectedNodes = SlateGraphEditor->GetSelectedNodes();
	SlateGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			SlateGraphEditor->SetNodeSelection(Node, true);
		}
	}

	// Delete the duplicatable nodes
	DeleteSelectedNodes();

	SlateGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			SlateGraphEditor->SetNodeSelection(Node, true);
		}
	}
}

bool FHoudiniBuildSequenceGraphEditor::CanCutNodes()
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FHoudiniBuildSequenceGraphEditor::CutSelectedNodes()
{
	CopySelectedNodes();
	DeleteSelectedDuplicatableNodes();
}

bool FHoudiniBuildSequenceGraphEditor::CanCopyNodes()
{
	// If any of the nodes can be duplicated then we should allow copying
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			return true;
		}
	}

	return false;
}

void FHoudiniBuildSequenceGraphEditor::CopySelectedNodes()
{
	// Export the selected nodes and place the text on the clipboard
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	FString ExportedText;

	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node == nullptr)
		{
			SelectedIter.RemoveCurrent();
			continue;
		}

		if (auto* EdNode_Edge = Cast<UEdNode_HoudiniBuildSequenceEdge>(*SelectedIter))
		{
			UEdNode_HoudiniBuildSequenceNode* StartNode = EdNode_Edge->GetStartNode();
			UEdNode_HoudiniBuildSequenceNode* EndNode = EdNode_Edge->GetEndNode();

			// Only copy an edge if both nodes it is connected to are also selected.
			if (!SelectedNodes.Contains(StartNode) || !SelectedNodes.Contains(EndNode))
			{
				SelectedIter.RemoveCurrent();
				continue;
			}
		}

		Node->PrepareForCopying();
	}

	FEdGraphUtilities::ExportNodesToText(SelectedNodes, ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);

	// TODO(): Determine if the node's owner needs to change. See FSoundCueEditor::CopySelectedNodes() -> PostCopyNode
}

bool FHoudiniBuildSequenceGraphEditor::CanPasteNodes()
{
	if (!SlateGraphEditor.IsValid())
	{
		return false;
	}

	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(SlateGraphEditor->GetCurrentGraph(), ClipboardContent);
}

void FHoudiniBuildSequenceGraphEditor::PasteNodes()
{
	if (!SlateGraphEditor.IsValid())
	{
		return;
	}

	PasteNodesHere(SlateGraphEditor->GetPasteLocation());
}

void FHoudiniBuildSequenceGraphEditor::PasteNodesHere(const FVector2D& Location)
{
	if (!SlateGraphEditor.IsValid())
	{
		return;
	}

	const FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());
	
	// TODO(): Go through the SoundCue code and make sure I call Modify() in the same places they do.
	// Undo/Redo support
	UEdGraph* EdGraph = SlateGraphEditor->GetCurrentGraph();
	EdGraph->Modify();
	SequenceGraph->Modify();

	// Clear the selection set (newly pasted stuff will be selected)
	SlateGraphEditor->ClearSelectionSet();

	// Grab the text to paste from the clipboard.
	FString TextToImport;
	FPlatformApplicationMisc::ClipboardPaste(TextToImport);

	// Import the nodes
	TSet<UEdGraphNode*> PastedNodes;
	FEdGraphUtilities::ImportNodesFromText(EdGraph, TextToImport, PastedNodes);

	//Average position of nodes so we can move them while still maintaining relative distances to each other
	FVector2D AvgNodePosition(0.0f, 0.0f);

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* Node = *It;
		AvgNodePosition.X += Node->NodePosX;
		AvgNodePosition.Y += Node->NodePosY;
	}

	float InvNumNodes = 1.0f / float(PastedNodes.Num());
	AvgNodePosition.X *= InvNumNodes;
	AvgNodePosition.Y *= InvNumNodes;

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* Node = *It;
		SlateGraphEditor->SetNodeSelection(Node, true);

		Node->NodePosX = (Node->NodePosX - AvgNodePosition.X) + Location.X;
		Node->NodePosY = (Node->NodePosY - AvgNodePosition.Y) + Location.Y;

		Node->SnapToGrid(16);

		// Give new node a different Guid from the old one
		Node->CreateNewGuid();
	}

	CastChecked<UEdGraph_HoudiniBuildSequenceGraph>(SequenceGraph->EditorGraph)->RebuildSequenceGraph();

	// Update UI
	SlateGraphEditor->NotifyGraphChanged();
	SequenceGraph->PostEditChange();
	SequenceGraph->MarkPackageDirty();
}

bool FHoudiniBuildSequenceGraphEditor::CanDuplicateNodes()
{
	return CanCopyNodes();
}

void FHoudiniBuildSequenceGraphEditor::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();
}

bool FHoudiniBuildSequenceGraphEditor::CanRenameNodes() const
{
	return GetSelectedNodes().Num() == 1;
}

void FHoudiniBuildSequenceGraphEditor::OnRenameNode()
{
	if (!SlateGraphEditor.IsValid())
	{
		return;
	}

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* SelectedNode = Cast<UEdGraphNode>(*NodeIt);
		if (SelectedNode != NULL && SelectedNode->bCanRenameNode)
		{
			SlateGraphEditor->IsNodeTitleVisible(SelectedNode, true);
			break;
		}
	}
}

TSharedRef<SDockTab> FHoudiniBuildSequenceGraphEditor::SpawnTab_GraphCanvas(const FSpawnTabArgs& Args)
{
	check( Args.GetTabId() == GraphCanvasTabId );

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("HBSGEditor_GraphCanvasTitle", "Viewport"));

	if (SlateGraphEditor.IsValid())
	{
		SpawnedTab->SetContent(SlateGraphEditor.ToSharedRef());
	}

	return SpawnedTab;
}

TSharedRef<SDockTab> FHoudiniBuildSequenceGraphEditor::SpawnTab_Properties(const FSpawnTabArgs& Args)
{
	check( Args.GetTabId() == PropertiesTabId );

	return SNew(SDockTab)
		.Label(LOCTEXT("BuildSequenceGraphDetailsTitle", "Details"))
		[
			NodeProperties.ToSharedRef()
		];
}

void FHoudiniBuildSequenceGraphEditor::SetSelectedManager(TWeakObjectPtr<AHoudiniBuildManager> NewSelectedManager)
{
	SelectedManager = NewSelectedManager;
	// TODO(): refresh the editor?
}

void FHoudiniBuildSequenceGraphEditor::ExtendToolbar()
{
	struct Local
	{
		static void FillToolBar(FToolBarBuilder& ToolBarBuilder, TWeakPtr<FHoudiniBuildSequenceGraphEditor> Editor)
		{
			ToolBarBuilder.BeginSection("ManagerSelectionToolbar");
			{
				ToolBarBuilder.AddWidget(SNew(SHoudiniBSGManagerSelectorWidget, Editor));
				
				ToolBarBuilder.AddToolBarButton(
					FHoudiniBSGEditorCommands::Get().ExecuteGraph,
					NAME_None,
					LOCTEXT("Playbutton_Label", "Run"),
					LOCTEXT("Playbutton_Tooltip", "Runs this graph for the currently selected BuildManager"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Play")
				);
			}
			ToolBarBuilder.EndSection();
		}
	};
	
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	
	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic(&Local::FillToolBar, SharedThis(this).ToWeakPtr())
	);

	AddToolbarExtender(ToolbarExtender);
}

bool FHoudiniBuildSequenceGraphEditor::CanExecuteGraph() const
{
	return SelectedManager.IsValid();
}

void FHoudiniBuildSequenceGraphEditor::ExecuteGraph()
{
	if (!SelectedManager.IsValid())
	{
		return;
	}

	SelectedManager->Run();
}

#undef LOCTEXT_NAMESPACE