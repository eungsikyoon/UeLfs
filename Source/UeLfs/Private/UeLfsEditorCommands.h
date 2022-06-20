// ----------------------------------------------------------------------------
// © 2022 Eungsik Yoon <yoon.eungsik@gmail.com>
// ----------------------------------------------------------------------------

#pragma once

//=======================================================================================
// Includes
//=======================================================================================
#include "Framework/Commands/Commands.h"
#include "EditorStyleSet.h"

//=======================================================================================
// FLuaBlueprintFunctionLibrary
//=======================================================================================

class FUeLfsEditorCommands : public TCommands<FUeLfsEditorCommands>
{
public:
	FUeLfsEditorCommands()
		: TCommands<FUeLfsEditorCommands>(TEXT("Unlock"), FText::FromString(TEXT("Unlock all assets.")), NAME_None, FEditorStyle::GetStyleSetName())
	{
	}

	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> UnlockButton;
};