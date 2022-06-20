// ----------------------------------------------------------------------------
// © 2022 Eungsik Yoon <yoon.eungsik@gmail.com>
// ----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "UeLfsSettings.h"
#include "UeLfsProvider.h"
#include "UeLfsUtils.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/SlateStyle.h"

class FUeLfsModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	FUeLfsSettings& AccessSettings();
	void SaveSettings();

	FUeLfsProvider& GetProvider();

	FUeLfsHttp& GetHttp();

	// Get all locked items by me.
	void GetMyLockedItems(TArray<FLfsLockItem>& OutLockedItems);

	// Unlock a single asset.
	bool UnlockSingleAsset(const FLfsLockItem& Ali);

private:

	/** The one and only Subversion source control provider */
	FUeLfsProvider UeLfsProvider;

	FUeLfsSettings UeLfsSettings;

	FUeLfsHttp UeLfsHttp;

	TSharedPtr<FSlateStyleSet> SlateStyleSet;

	class FUnlockIcon
	{
	public:
		FSlateIcon  GetIcon() const;
		FText       GetTooltip() const;
	};
	TSharedPtr<FUnlockIcon> UnlockIconPtr;

private: // For custom 'unlock' button.

	TSharedPtr<FExtender> ToolbarExtender;

	void AddToolbarExtension(FToolBarBuilder& ToolbarBuilder);

	void OnUnlockButtonClicked();
};
