// ----------------------------------------------------------------------------
// © 2022 Eungsik Yoon <yoon.eungsik@gmail.com>
// ----------------------------------------------------------------------------

#include "UeLfsEditorCommands.h"
#include "Framework/Commands/Commands.h"

//=======================================================================================
// FUeLfsEditorCommands
//=======================================================================================
PRAGMA_DISABLE_OPTIMIZATION
void FUeLfsEditorCommands::RegisterCommands()
{
#define LOCTEXT_NAMESPACE "UeLfsEditorCommand"

	UI_COMMAND(UnlockButton, "Unlock", "Unlock all assets.",
		EUserInterfaceActionType::Button,
		FInputChord(EModifierKey::Alt, EKeys::U));

#undef LOCTEXT_NAMESPACE
}
PRAGMA_ENABLE_OPTIMIZATION
