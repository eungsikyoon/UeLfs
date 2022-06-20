// ----------------------------------------------------------------------------
// © 2022 Eungsik Yoon <yoon.eungsik@gmail.com>
// ----------------------------------------------------------------------------

#include "UeLfsOperations.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"
#include "Modules/ModuleManager.h"
#include "SourceControlOperations.h"
#include "ISourceControlModule.h"
#include "UeLfsCommand.h"
#include "UeLfsModule.h"

#define LOCTEXT_NAMESPACE "UeLfs"

//-----------------------------------------------------------------------------
// "Connect" worker impl.
//-----------------------------------------------------------------------------

FName FUeLfsWorkerConnect::GetName() const
{
	return "Connect";
}

bool FUeLfsWorkerConnect::Execute(FUeLfsCommand& InCommand)
{
	check(InCommand.Operation->GetName() == GetName());

	InCommand.bCommandSuccessful = true;

	// Configure http connector.
	FUeLfsModule& UeLfs = FModuleManager::GetModuleChecked<FUeLfsModule>("UeLfs");
	FUeLfsSettings& Settings = UeLfs.AccessSettings();
	FUeLfsHttp& AlHttp = UeLfs.GetHttp();
	AlHttp.Configure(Settings);

	TArray<FString> LockedGitPaths;
	bool bOk = AlHttp.ReqLogin(LockedGitPaths);

	if (bOk)
	{
		const FString& UserName = Settings.GetUserName();
		const FString& RepoRootPath = Settings.GetRepoRootPath();

		for (const FString& GitPath : LockedGitPaths)
		{
			const FString LocalFilePath = FPaths::Combine(RepoRootPath, GitPath);

			FLfsLockInfo Ali;
			Ali.FilePath = LocalFilePath;
			Ali.LockUserName = UserName;

			LockInfos.Emplace(Ali);
		}
	}

	InCommand.bCommandSuccessful = bOk;
	return InCommand.bCommandSuccessful;
}

bool FUeLfsWorkerConnect::UpdateStates() const
{
	FUeLfsModule& UeLfs = FModuleManager::GetModuleChecked<FUeLfsModule>("UeLfs");
	FUeLfsProvider& Provider = UeLfs.GetProvider();
	Provider.UpdateLockedStates(LockInfos);
	return true;
}

//-----------------------------------------------------------------------------
// "UpdateStatus" worker impl.
//-----------------------------------------------------------------------------

FName FUeLfsWorkerUpdateStatus::GetName() const
{
	return "UpdateStatus";
}

//-----------------------------------------------------------------------------
bool FUeLfsWorkerUpdateStatus::Execute(FUeLfsCommand& InCommand)
{
	// update using any special hints passed in via the operation
	check(InCommand.Operation->GetName() == GetName());

	InCommand.bCommandSuccessful = true;

	if (InCommand.Files.Num() == 0)
	{
		UE_LOG(LogSourceControl, Warning, TEXT("[TEMP] UpdateStatus with zero files."));
		return InCommand.bCommandSuccessful;
	}

	// First update lock states.
	FUeLfsModule& UeLfs = FModuleManager::GetModuleChecked<FUeLfsModule>("UeLfs");
	FUeLfsHttp& AlHttp = UeLfs.GetHttp();
	bool bOk = AlHttp.ReqGetLockStates(InCommand.Files, LockInfos);

	if (bOk)
	{
		const FString& RepoRootPath = UeLfs.AccessSettings().GetRepoRootPath();
		bOk = UeLfsUtils::GetAddedFiles(InCommand.Files, RepoRootPath, AddedFiles);
	}

	InCommand.bCommandSuccessful = bOk;
	return InCommand.bCommandSuccessful;
}

//-----------------------------------------------------------------------------
bool FUeLfsWorkerUpdateStatus::UpdateStates() const
{
	FUeLfsModule& UeLfs = FModuleManager::GetModuleChecked<FUeLfsModule>("UeLfs");
	FUeLfsProvider& Provider = UeLfs.GetProvider();
	Provider.UpdateLockedStates(LockInfos);
	Provider.UpdateAddedStates(AddedFiles);
	return true;
}

//-----------------------------------------------------------------------------
// "CheckOut" worker impl.
//-----------------------------------------------------------------------------

FName FUeLfsWorkerCheckOut::GetName() const
{
	return "CheckOut";
}

//-----------------------------------------------------------------------------
bool FUeLfsWorkerCheckOut::Execute(FUeLfsCommand& InCommand)
{
	UE_LOG(LogSourceControl, Warning, TEXT("[zz] FUeLfsWorkerCheckOut::Execute"));

	// update using any special hints passed in via the operation
	check(InCommand.Operation->GetName() == GetName());

	InCommand.bCommandSuccessful = true;

	if (InCommand.Files.Num() == 0)
	{
		UE_LOG(LogSourceControl, Warning, TEXT("[TEMP] CheckOut with zero files."));
		return InCommand.bCommandSuccessful;
	}

	FUeLfsModule& UeLfs = FModuleManager::GetModuleChecked<FUeLfsModule>("UeLfs");
	FUeLfsProvider& Provider = UeLfs.GetProvider();
	TArray<FLfsLockItem> LockItems =
		Provider.MakeLockItems(UeLfs.AccessSettings().GetRepoRootPath(), InCommand.Files);
	FUeLfsHttp& AlHttp = UeLfs.GetHttp();
	bool bOk = AlHttp.ReqLockFiles(LockItems, LockInfos);

	if (bOk)
	{
		// Comment copied from FSubversionCheckOutWorker::Execute().
		// "Annoyingly, we need remove any read-only flags here (for cross-working with Perforce)"
		for (auto Iter(InCommand.Files.CreateConstIterator()); Iter; Iter++)
		{
			FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(**Iter, false);

			ModifiedFiles.Add(**Iter);
		}
	}

	InCommand.bCommandSuccessful = bOk;
	return InCommand.bCommandSuccessful;
}

//-----------------------------------------------------------------------------
bool FUeLfsWorkerCheckOut::UpdateStates() const
{
	FUeLfsModule& UeLfs = FModuleManager::GetModuleChecked<FUeLfsModule>("UeLfs");
	FUeLfsProvider& Provider = UeLfs.GetProvider();
	Provider.UpdateLockedStates(LockInfos);
	Provider.UpdateModifiedStates(ModifiedFiles);
	return true;
}

//-----------------------------------------------------------------------------
// "CheckIn" worker impl.
//-----------------------------------------------------------------------------

FName FUeLfsWorkerCheckIn::GetName() const
{
	return "CheckIn";
}

//-----------------------------------------------------------------------------
bool FUeLfsWorkerCheckIn::Execute(FUeLfsCommand& InCommand)
{
	check(InCommand.Operation->GetName() == GetName());

	InCommand.bCommandSuccessful = true;

	if (InCommand.Files.Num() == 0)
	{
		UE_LOG(LogSourceControl, Warning, TEXT("[TEMP] Revert with zero files."));
		return InCommand.bCommandSuccessful;
	}

	FUeLfsModule& UeLfs = FModuleManager::GetModuleChecked<FUeLfsModule>("UeLfs");
	FUeLfsProvider& Provider = UeLfs.GetProvider();
	TArray<FLfsLockItem> LockItems =
		Provider.MakeLockItems(UeLfs.AccessSettings().GetRepoRootPath(), InCommand.Files);
	FUeLfsHttp& AlHttp = UeLfs.GetHttp();
	bool bOk = AlHttp.ReqUnlockFiles(LockItems, LockInfos);

	InCommand.bCommandSuccessful = bOk;
	return InCommand.bCommandSuccessful;
}

//-----------------------------------------------------------------------------
bool FUeLfsWorkerCheckIn::UpdateStates() const
{
	FUeLfsModule& UeLfs = FModuleManager::GetModuleChecked<FUeLfsModule>("UeLfs");
	FUeLfsProvider& Provider = UeLfs.GetProvider();
	Provider.UpdateLockedStates(LockInfos);
	return true;
}

//-----------------------------------------------------------------------------
// "Delete" worker impl.
//-----------------------------------------------------------------------------

FName FUeLfsWorkerDelete::GetName() const
{
	return "Delete";
}

//-----------------------------------------------------------------------------
bool FUeLfsWorkerDelete::Execute(FUeLfsCommand& InCommand)
{
	// update using any special hints passed in via the operation
	check(InCommand.Operation->GetName() == GetName());

	InCommand.bCommandSuccessful = true;

	if (InCommand.Files.Num() == 0)
	{
		UE_LOG(LogSourceControl, Warning, TEXT("[TEMP] Delete with zero files."));
		return InCommand.bCommandSuccessful;
	}

	FUeLfsModule& UeLfs = FModuleManager::GetModuleChecked<FUeLfsModule>("UeLfs");
	FUeLfsProvider& Provider = UeLfs.GetProvider();
	TArray<FLfsLockItem> LockItems =
		Provider.MakeLockItems(UeLfs.AccessSettings().GetRepoRootPath(), InCommand.Files);
	FUeLfsHttp& AlHttp = UeLfs.GetHttp();
	bool bOk = AlHttp.ReqLockFiles(LockItems, LockInfos);

	if (bOk)
	{
		for (auto Iter(InCommand.Files.CreateConstIterator()); Iter; Iter++)
		{
			if (!FPlatformFileManager::Get().GetPlatformFile().DeleteFile(**Iter))
			{
				UE_LOG(LogSourceControl, Error, TEXT("Failed to delete file: %s"), **Iter);
			}

			// Save deleted files.
			DeletedFiles.Add(**Iter);
		}
	}

	InCommand.bCommandSuccessful = bOk;
	return InCommand.bCommandSuccessful;
}

//-----------------------------------------------------------------------------
bool FUeLfsWorkerDelete::UpdateStates() const
{
	FUeLfsModule& UeLfs = FModuleManager::GetModuleChecked<FUeLfsModule>("UeLfs");
	FUeLfsProvider& Provider = UeLfs.GetProvider();
	Provider.UpdateLockedStates(LockInfos);
	Provider.UpdateDeletedFiles(DeletedFiles);
	return true;
}

//-----------------------------------------------------------------------------
// "Revert" worker impl. (dummy)
//-----------------------------------------------------------------------------

FName FUeLfsWorkerRevert::GetName() const
{
	return "Revert";
}

//-----------------------------------------------------------------------------
bool FUeLfsWorkerRevert::Execute(FUeLfsCommand& InCommand)
{
	InCommand.bCommandSuccessful = true;
	return InCommand.bCommandSuccessful;
}

//-----------------------------------------------------------------------------
bool FUeLfsWorkerRevert::UpdateStates() const
{
	return true;
}

//-----------------------------------------------------------------------------
// "Copy" worker impl. (dummy)
//-----------------------------------------------------------------------------

FName FUeLfsWorkerCopy::GetName() const
{
	return "Copy";
}

//-----------------------------------------------------------------------------
bool FUeLfsWorkerCopy::Execute(FUeLfsCommand& InCommand)
{
	InCommand.bCommandSuccessful = true;
	return InCommand.bCommandSuccessful;
}

//-----------------------------------------------------------------------------
bool FUeLfsWorkerCopy::UpdateStates() const
{
	return true;
}

//-----------------------------------------------------------------------------
// "MarkForAdd" worker impl. (dummy)
//-----------------------------------------------------------------------------

FName FUeLfsWorkerMarkForAdd::GetName() const
{
	return "MarkForAdd";
}

//-----------------------------------------------------------------------------
bool FUeLfsWorkerMarkForAdd::Execute(FUeLfsCommand& InCommand)
{
	InCommand.bCommandSuccessful = true;
	return InCommand.bCommandSuccessful;
}

//-----------------------------------------------------------------------------
bool FUeLfsWorkerMarkForAdd::UpdateStates() const
{
	return true;
}

#undef LOCTEXT_NAMESPACE // "UeLfs"
