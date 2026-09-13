// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "SBSBlueprintMigrationLibrary.h"

#include "BlueprintFunctionLib/SBSApiFunctionLibrary.h"
#include "Dom/JsonObject.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphUtilities.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonSerializer.h"

int32 USBSBlueprintMigrationLibrary::MigrateImageUrls(UBlueprint* Blueprint)
{
	if (!IsValid(Blueprint))
	{
		return -1;
	}
	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	TArray<UEdGraphPin*> Pins;
	for (UEdGraph* Graph : Graphs)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin->Direction == EGPD_Input && Pin->LinkedTo.IsEmpty() &&
					Pin->DefaultValue == TEXT("https://sbs.kyrium.space/api/v1/image/"))
				{
					Pins.Add(Pin);
				}
			}
		}
	}
	for (UEdGraphPin* Pin : Pins)
	{
		UEdGraphNode* Target = Pin->GetOwningNode();
		UEdGraph* Graph = Target->GetGraph();
		UK2Node_CallFunction* Url = NewObject<UK2Node_CallFunction>(Graph);
		Url->SetFromFunction(USBSApiFunctionLibrary::StaticClass()->FindFunctionByName(
			GET_FUNCTION_NAME_CHECKED(USBSApiFunctionLibrary, GetImageBaseUrl)));
		Graph->AddNode(Url, false, false);
		Url->CreateNewGuid();
		Url->AllocateDefaultPins();
		Url->NodePosX = Target->NodePosX - 300;
		Url->NodePosY = Target->NodePosY + 96;
		if (!Graph->GetSchema()->TryCreateConnection(Url->GetReturnValuePin(), Pin))
		{
			return -1;
		}
		Pin->DefaultValue.Empty();
	}
	if (!Pins.IsEmpty())
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}
	return Pins.Num();
}

FString USBSBlueprintMigrationLibrary::InspectBlueprint(UBlueprint* Blueprint)
{
	if (!IsValid(Blueprint))
	{
		return TEXT("{\"errors\":1,\"status\":\"missing\"}");
	}
	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipGarbageCollection, &Results);
	const TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetStringField(TEXT("asset"), Blueprint->GetPathName());
	Report->SetNumberField(TEXT("errors"), Results.NumErrors);
	Report->SetNumberField(TEXT("warnings"), Results.NumWarnings);
	Report->SetStringField(TEXT("status"), Blueprint->Status == BS_Error ? TEXT("error") : TEXT("compiled"));
	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	const TSharedRef<FJsonObject> Exports = MakeShared<FJsonObject>();
	for (UEdGraph* Graph : Graphs)
	{
		TSet<UObject*> Nodes;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			Nodes.Add(Node);
		}
		FString Export;
		FEdGraphUtilities::ExportNodesToText(Nodes, Export);
		Exports->SetStringField(Graph->GetName(), Export);
	}
	Report->SetObjectField(TEXT("graphs"), Exports);
	FString Text;
	FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Text));
	return Text;
}

IMPLEMENT_MODULE(FDefaultModuleImpl, SBSEditor);
