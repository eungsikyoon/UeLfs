// ----------------------------------------------------------------------------
// © 2022 Eungsik Yoon <yoon.eungsik@gmail.com>
// ----------------------------------------------------------------------------

#include "UeLfsModule.h"
#include "Misc/App.h"
#include "Modules/ModuleManager.h"
#include "UeLfsOperations.h"
#include "Features/IModularFeatures.h"
#include "LevelEditor.h"
#include "ISourceControlModule.h"
#include "UeLfsEditorCommands.h"
#include "Misc/MessageDialog.h"
#include "PackagesDialog.h"
#include "Interfaces/IPluginManager.h"
#include "Brushes/SlateImageBrush.h"
#include "Slate/SlateGameResources.h"
#include "Styling/SlateStyleRegistry.h"

#define LOCTEXT_NAMESPACE "UeLfs"

template<typename Type>
static TSharedRef<IUeLfsWorker, ESPMode::ThreadSafe> CreateWorker()
{
	return MakeShareable( new Type() );
}

void FUeLfsModule::StartupModule()
{
	// Register operations.
	UeLfsProvider.RegisterWorker("Connect",
		FGetUeLfsWorker::CreateStatic( &CreateWorker<FUeLfsWorkerConnect> ));
	UeLfsProvider.RegisterWorker("UpdateStatus",
		FGetUeLfsWorker::CreateStatic( &CreateWorker<FUeLfsWorkerUpdateStatus> ));
	UeLfsProvider.RegisterWorker("CheckOut",
		FGetUeLfsWorker::CreateStatic(&CreateWorker<FUeLfsWorkerCheckOut>));
	UeLfsProvider.RegisterWorker("CheckIn",
		FGetUeLfsWorker::CreateStatic(&CreateWorker<FUeLfsWorkerCheckIn>));
	UeLfsProvider.RegisterWorker("Delete",
		FGetUeLfsWorker::CreateStatic(&CreateWorker<FUeLfsWorkerDelete>));

	// Dummy operations.
	UeLfsProvider.RegisterWorker("Revert",
		FGetUeLfsWorker::CreateStatic(&CreateWorker<FUeLfsWorkerRevert>));
	UeLfsProvider.RegisterWorker("Copy",
		FGetUeLfsWorker::CreateStatic(&CreateWorker<FUeLfsWorkerCopy>));
	UeLfsProvider.RegisterWorker("MarkForAdd",
		FGetUeLfsWorker::CreateStatic(&CreateWorker<FUeLfsWorkerMarkForAdd>));

	UeLfsSettings.LoadSettings();

	IModularFeatures::Get().RegisterModularFeature("SourceControl", &UeLfsProvider);

	// Register the style for the 'unlock' button icon.
	FString RootPath(IPluginManager::Get().FindPlugin(TEXT("UeLfs"))->GetBaseDir());
	FString ResourcesPath = RootPath / TEXT("Content") / TEXT("Icons");
	SlateStyleSet = FSlateGameResources::New("UeLfs", ResourcesPath, ResourcesPath);

	FString IconPath = SlateStyleSet->RootToContentDir("unlock", TEXT(".png"));
	SlateStyleSet->Set("UeLfs.Unlock", new FSlateImageBrush(IconPath, FVector2D(40.0f, 40.0f)));
	SlateStyleSet->Set("UeLfs.Unlock.Small", new FSlateImageBrush(IconPath, FVector2D(20.0f, 20.0f)));
	FSlateStyleRegistry::RegisterSlateStyle(*SlateStyleSet);

	UnlockIconPtr = MakeShareable(new FUnlockIcon());

	// Register the 'unlock' button.
	FUeLfsEditorCommands::Register();

	const TSharedRef< FUICommandList > PluginCommands = MakeShareable(new FUICommandList);
	PluginCommands->MapAction(
		FUeLfsEditorCommands::Get().UnlockButton,
		FExecuteAction::CreateRaw(this, &FUeLfsModule::OnUnlockButtonClicked),
		FCanExecuteAction());

	FLevelEditorModule& LevelEditorModule =
		FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension("File",
		EExtensionHook::After,
		PluginCommands,
		FToolBarExtensionDelegate::CreateRaw(this, &FUeLfsModule::AddToolbarExtension));

	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
}

void FUeLfsModule::ShutdownModule()
{
	// Shut down the provider, as this module is going away.
	UeLfsProvider.Close();

	// Unbind provider from editor.
	IModularFeatures::Get().UnregisterModularFeature("SourceControl", &UeLfsProvider);

	// Unregister the 'unlock' button icon.
	UnlockIconPtr = nullptr;
	FSlateStyleRegistry::UnRegisterSlateStyle(*SlateStyleSet);
	ensure(SlateStyleSet.IsUnique());
	SlateStyleSet.Reset();

	// Unregister the 'unlock' button.
	FLevelEditorModule& LevelEditorModule =
		FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetToolBarExtensibilityManager()->RemoveExtender(ToolbarExtender);
}

FUeLfsSettings& FUeLfsModule::AccessSettings()
{
	return UeLfsSettings;
}

void FUeLfsModule::SaveSettings()
{
	if (FApp::IsUnattended() || IsRunningCommandlet())
	{
		return;
	}

	UeLfsSettings.SaveSettings();
}

FUeLfsProvider& FUeLfsModule::GetProvider()
{
	return UeLfsProvider;
}

FUeLfsHttp& FUeLfsModule::GetHttp()
{
	return UeLfsHttp;
}

void FUeLfsModule::GetMyLockedItems(TArray<FLfsLockItem>& OutLockedItems)
{
	const FString& MyUserName = UeLfsSettings.GetUserName();

	UeLfsProvider.GetLockedFiles(OutLockedItems, MyUserName);
}

bool FUeLfsModule::UnlockSingleAsset(const FLfsLockItem& Ali)
{
	UE_LOG(LogSourceControl, Warning, TEXT("[UeLfs] Unlocking all assets ..."));

	if (!UeLfsHttp.IsLoggedIn())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("UeLfs_UnlockMsg", "You are not logged in!"));
		return false;
	}

	TArray<FLfsLockInfo> LockUpdates;
	TArray<FLfsLockItem> AliArray;
	AliArray.Emplace(Ali);

	bool bOk = UeLfsHttp.ReqUnlockFiles(AliArray, LockUpdates);
	if (!bOk)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("UeLfs_UnlockMsg", "Failed to release lock!"));
		return false;
	}

	UeLfsProvider.UpdateLockedStates(LockUpdates);

	FText MsgTxt = FText::FromString(FString::Printf(TEXT("Lock released for [%s]"), *Ali.GitFilePath));
	FMessageDialog::Open(EAppMsgType::Ok, MsgTxt);
	return true;
}

FSlateIcon FUeLfsModule::FUnlockIcon::GetIcon() const
{
	return FSlateIcon("UeLfs", "UeLfs.Unlock", "UeLfs.Unlock.Small");
}

FText FUeLfsModule::FUnlockIcon::GetTooltip() const
{
	return LOCTEXT("UeLfs_UnlockLabel", "Unlock");
}

void FUeLfsModule::AddToolbarExtension(FToolBarBuilder& ToolbarBuilder)
{
	ToolbarBuilder.AddToolBarButton(
		FUeLfsEditorCommands::Get().UnlockButton,
		NAME_None,
		LOCTEXT("UeLfs_UnlockLabel", "Unlock"),
		TAttribute<FText>(UnlockIconPtr.ToSharedRef(), &FUnlockIcon::GetTooltip),
		TAttribute<FSlateIcon>(UnlockIconPtr.ToSharedRef(), &FUnlockIcon::GetIcon),
		NAME_None);
}

void FUeLfsModule::OnUnlockButtonClicked()
{
	UE_LOG(LogSourceControl, Warning, TEXT("[UeLfs] Unlocking assets ..."));

	if (!UeLfsHttp.IsLoggedIn())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("UeLfs_UnlockMsg", "You are not logged in!"));
		return;
	}

	TArray<FLfsLockItem> LockedItems;
	UeLfsProvider.GetLockedFiles(LockedItems, UeLfsSettings.GetUserName());
	if (LockedItems.Num() > 0)
	{
		FPackagesDialogModule& PackagesDialogModule = FModuleManager::LoadModuleChecked<FPackagesDialogModule>(TEXT("PackagesDialog"));
		PackagesDialogModule.CreatePackagesDialog(NSLOCTEXT("PackagesDialogModule", "PackagesDialogTitle", "Unlock Content"), NSLOCTEXT("PackagesDialogModule", "PackagesDialogMessage", "Select content to unlock."));
		PackagesDialogModule.AddButton(DRT_Save, NSLOCTEXT("PackagesDialogModule", "UnlockSelectedButton", "Unlock Selected"), NSLOCTEXT("PackagesDialogModule", "UnlockSelectedButtonTip", "Attempt to unlock the selected content"));
		PackagesDialogModule.AddButton(DRT_Cancel, NSLOCTEXT("PackagesDialogModule", "CancelButton", "Cancel"), NSLOCTEXT("PackagesDialogModule", "CancelButtonTip", "Do not unlock any content and cancel the current operation"));

		for (const FLfsLockItem& Item: LockedItems)
		{
			FString PackageName = FPackageName::FilenameToLongPackageName(Item.LocalFilePath);
			UObject* Obj = nullptr;
			UPackage* MockPackage = nullptr;
			Obj = StaticFindObjectFast(UPackage::StaticClass(), nullptr, *PackageName, true);
			if (Obj && Obj->IsA(UPackage::StaticClass()))
			{
				MockPackage = Cast<UPackage>(Obj);
			}
			else
			{
				MockPackage = LoadPackage(nullptr, *PackageName, LOAD_None);
			}

			//삭제한 경우
			if (!MockPackage)
			{
				MockPackage = NewObject<UPackage>(nullptr, *PackageName, RF_Transient);
			}

			MockPackage->FileName = *Item.LocalFilePath;
			PackagesDialogModule.AddPackageItem(MockPackage, ECheckBoxState::Unchecked);
		}

		TSet<FString> IgnoredPackages;
		const EDialogReturnType UserResponse = PackagesDialogModule.ShowPackagesDialog(IgnoredPackages);

		if (UserResponse == DRT_Save)
		{
			TArray<UPackage*> PkgsToUnlock;
			PackagesDialogModule.GetResults(PkgsToUnlock, ECheckBoxState::Checked);

			TArray<FLfsLockItem> SelectedItems;
			for (UPackage* Package : PkgsToUnlock)
			{
				FString FilePath = Package->FileName.ToString();
				for (const FLfsLockItem& Item : LockedItems)
				{
					if (Item.LocalFilePath == FilePath)
					{
						UE_LOG(LogSourceControl, Display, TEXT("Selected to unlock : %s"), *FilePath);
						SelectedItems.Emplace(Item);
						break;
					}
				}
			}

			if (SelectedItems.Num() > 0)
			{
				TArray<FLfsLockInfo> LockUpdates;
				bool bOk = UeLfsHttp.ReqUnlockFiles(SelectedItems, LockUpdates);
				if (!bOk)
				{
					FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("UeLfs_UnlockMsg", "Failed to release lock!"));
					return;
				}

				UeLfsProvider.UpdateLockedStates(LockUpdates);

				FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("UeLfs_UnlockMsg", "All selected locks released!"));
			}
		}
	}
}

IMPLEMENT_MODULE(FUeLfsModule, UeLfs);

#undef LOCTEXT_NAMESPACE // "UeLfs"
