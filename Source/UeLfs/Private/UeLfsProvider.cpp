// ----------------------------------------------------------------------------
// © 2022 Eungsik Yoon <yoon.eungsik@gmail.com>
// ----------------------------------------------------------------------------

#include "UeLfsProvider.h"
#include "Misc/MessageDialog.h"
#include "Misc/QueuedThreadPool.h"
#include "Modules/ModuleManager.h"
#include "ISourceControlModule.h"
#include "UeLfsCommand.h"
#include "UeLfsModule.h"
#include "SourceControlHelpers.h"
#include "SourceControlOperations.h"
#include "UeLfsUtils.h"
#include "SUeLfsSettings.h"
#include "Logging/MessageLog.h"
#include "ScopedSourceControlProgress.h"

#define LOCTEXT_NAMESPACE "UeLfs"

void FUeLfsProvider::Init(bool bForceConnection)
{
	// Set git binary and repo root path if not set already.
	FUeLfsModule& UeLfs = FModuleManager::GetModuleChecked<FUeLfsModule>("UeLfs");
	const FString& GitBinaryPath = UeLfs.AccessSettings().GetGitBinaryPath();
	if (GitBinaryPath.IsEmpty())
	{
		FString BinPath = UeLfsUtils::FindGitBinaryPath();
		UeLfs.AccessSettings().SetGitBinaryPath(BinPath);
	}

	const FString& RepoRootPath = UeLfs.AccessSettings().GetRepoRootPath();
	if (RepoRootPath.IsEmpty())
	{
		FString RootPath;
		FString ProjectDirAbs = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
		UeLfsUtils::FindRepoRootPath(ProjectDirAbs, RootPath);
		UeLfs.AccessSettings().SetRepoRootPath(RootPath);
	}

	// Recommend the "default" asset locker server.
	const FString& ServerUrl = UeLfs.AccessSettings().GetServerUrl();
	if (ServerUrl.IsEmpty())
	{
		UeLfs.AccessSettings().SetServerUrl(TEXT("http://localhost:15111"));
	}

	// User "git config user.name" by default.
	const FString& UserName = UeLfs.AccessSettings().GetUserName();
	if (UserName.IsEmpty())
	{
		FString GitUserName = UeLfsUtils::GetGitUserName();
		UeLfs.AccessSettings().SetUserName(GitUserName);
	}
}

void FUeLfsProvider::Close()
{
}

FText FUeLfsProvider::GetStatusText() const
{
	//FFormatNamedArguments Args;
	//Args.Add( TEXT("IsEnabled"), IsEnabled() ? LOCTEXT("Yes", "Yes") : LOCTEXT("No", "No") );
	//Args.Add( TEXT("RepositoryName"), FText::FromString( GetRepositoryName() ) );
	//Args.Add( TEXT("UserName"), FText::FromString( GetUserName() ) );

	//return FText::Format( NSLOCTEXT("Status", "Provider: Subversion\nEnabledLabel", "Enabled: {IsEnabled}\nRepository: {RepositoryName}\nUser name: {UserName}"), Args );
	return FText::GetEmpty();
}

bool FUeLfsProvider::IsEnabled() const
{
	return true;
}

bool FUeLfsProvider::IsAvailable() const
{
	return true;
}

const FName& FUeLfsProvider::GetName(void) const
{
	static FName ProviderName("UeLfs");
	return ProviderName;
}

bool FUeLfsProvider::QueryStateBranchConfig(const FString& ConfigSrc, const FString& ConfigDest)
{
	return false;
}

void FUeLfsProvider::RegisterStateBranches(
	const TArray<FString>& BranchNames,
	const FString& ContentRoot)
{
}

ECommandResult::Type FUeLfsProvider::GetState(
	const TArray<FString>& InFiles,
	TArray< TSharedRef<ISourceControlState, ESPMode::ThreadSafe> >& OutState,
	EStateCacheUsage::Type InStateCacheUsage)
{
	check(IsEnabled());

	TArray<FString> AbsoluteFiles = SourceControlHelpers::AbsoluteFilenames(InFiles);
	if (InStateCacheUsage == EStateCacheUsage::ForceUpdate)
	{
		Execute(ISourceControlOperation::Create<FUpdateStatus>(), AbsoluteFiles);
	}

	for (TArray<FString>::TConstIterator It(AbsoluteFiles); It; It++)
	{
		TSharedRef<FUeLfsState, ESPMode::ThreadSafe> State = GetSingleState(*It);
		OutState.Add(State);
	}

	return ECommandResult::Succeeded;
}

TArray<FSourceControlStateRef> FUeLfsProvider::GetCachedStateByPredicate(TFunctionRef<bool(const FSourceControlStateRef&)> Predicate) const
{
	TArray<FSourceControlStateRef> Result;
	return Result;
}

FDelegateHandle FUeLfsProvider::RegisterSourceControlStateChanged_Handle( const FSourceControlStateChanged::FDelegate& SourceControlStateChanged )
{
	return OnSourceControlStateChanged.Add( SourceControlStateChanged );
}

void FUeLfsProvider::UnregisterSourceControlStateChanged_Handle( FDelegateHandle Handle )
{
	OnSourceControlStateChanged.Remove( Handle );
}

ECommandResult::Type FUeLfsProvider::Execute(
	const TSharedRef<ISourceControlOperation, ESPMode::ThreadSafe>& InOperation,
	const TArray<FString>& InFiles,
	EConcurrency::Type InConcurrency,
	const FSourceControlOperationComplete& InOperationCompleteDelegate )
{
	check(IsEnabled());

	UE_LOG(LogSourceControl, Warning, TEXT("[UeLfs-Exec]: %s (Async: %d, Files: %d)"),
		*InOperation->GetName().ToString(),
		(int32)InConcurrency,
		InFiles.Num());

	if (!UeLfsUtils::CheckFilenames(InFiles))
	{
		InOperation->AddErrorMessge(LOCTEXT("UnsupportedFile", "Filename with wildcards is not supported by UeLfs"));
		InOperationCompleteDelegate.ExecuteIfBound(InOperation, ECommandResult::Failed);
		return ECommandResult::Failed;
	}

	TArray<FString> AbsoluteFiles = SourceControlHelpers::AbsoluteFilenames(InFiles);

	// Query to see if the we allow this operation
	TSharedPtr<IUeLfsWorker, ESPMode::ThreadSafe> Worker = CreateWorker(InOperation->GetName());
	if (!Worker.IsValid())
	{
		// This operation is unsupported by this source control provider
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("OperationName"), FText::FromName(InOperation->GetName()));
		Arguments.Add(TEXT("ProviderName"), FText::FromName(GetName()));
		FText Message(FText::Format(LOCTEXT("UnsupportedOperation", "Operation '{OperationName}' not supported by source control provider '{ProviderName}'"), Arguments));
		FMessageLog("SourceControl").Error(Message);
		InOperation->AddErrorMessge(Message);
		InOperationCompleteDelegate.ExecuteIfBound(InOperation, ECommandResult::Failed);
		return ECommandResult::Failed;
	}

	// Fire off operation.
	if (InConcurrency == EConcurrency::Synchronous)
	{
		FUeLfsCommand* Command = new FUeLfsCommand(InOperation, Worker.ToSharedRef());
		Command->bAutoDelete = false;
		Command->Files = AbsoluteFiles;
		Command->OperationCompleteDelegate = InOperationCompleteDelegate;
		return ExecuteSynchronousCommand(*Command, InOperation->GetInProgressString(), true);
	}
	else
	{
		FUeLfsCommand* Command = new FUeLfsCommand(InOperation, Worker.ToSharedRef());
		Command->bAutoDelete = true;
		Command->Files = AbsoluteFiles;
		Command->OperationCompleteDelegate = InOperationCompleteDelegate;
		return IssueCommand(*Command, false);
	}
}

bool FUeLfsProvider::CanCancelOperation( const TSharedRef<ISourceControlOperation, ESPMode::ThreadSafe>& InOperation ) const
{
	return false;
}

void FUeLfsProvider::CancelOperation( const TSharedRef<ISourceControlOperation, ESPMode::ThreadSafe>& InOperation )
{
}

bool FUeLfsProvider::UsesLocalReadOnlyState() const
{
	return false;
}

bool FUeLfsProvider::UsesChangelists() const
{
	return false;
}

bool FUeLfsProvider::UsesCheckout() const
{
	return true;
}

void FUeLfsProvider::Tick()
{
	bool bStatesUpdated = false;
	for (int32 CommandIndex = 0; CommandIndex < CommandQueue.Num(); ++CommandIndex)
	{
		FUeLfsCommand& Command = *CommandQueue[CommandIndex];
		if (Command.bExecuteProcessed)
		{
			// Remove command from the queue
			CommandQueue.RemoveAt(CommandIndex);

			// let command update the states of any files
			bStatesUpdated |= Command.Worker->UpdateStates();

			// dump any messages to output log
			OutputCommandMessages(Command);

			Command.ReturnResults();

			//commands that are left in the array during a tick need to be deleted
			if (Command.bAutoDelete)
			{
				// Only delete commands that are not running 'synchronously'
				delete &Command;
			}

			// only do one command per tick loop, as we dont want concurrent modification
			// of the command queue (which can happen in the completion delegate)
			break;
		}
	}

	if (bStatesUpdated)
	{
		OnSourceControlStateChanged.Broadcast();
	}
}

TArray< TSharedRef<ISourceControlLabel> > FUeLfsProvider::GetLabels( const FString& InMatchingSpec ) const
{
	TArray< TSharedRef<ISourceControlLabel> > Labels;
	return Labels;
}

#if SOURCE_CONTROL_WITH_SLATE
TSharedRef<class SWidget> FUeLfsProvider::MakeSettingsWidget() const
{
	return SNew(SUeLfsSettings);
}
#endif // SOURCE_CONTROL_WITH_SLATE


const FString& FUeLfsProvider::GetUserName() const
{
	FUeLfsModule& UeLfs = FModuleManager::GetModuleChecked<FUeLfsModule>("UeLfs");
	return UeLfs.AccessSettings().GetUserName();
}


void FUeLfsProvider::RegisterWorker(const FName& InName, const FGetUeLfsWorker& InDelegate)
{
	WorkersMap.Add(InName, InDelegate);
}

void FUeLfsProvider::UpdateLockedStates(const TArray<FLfsLockInfo>& LockInfos)
{
	for (const FLfsLockInfo& Ali : LockInfos)
	{
		TSharedRef<FUeLfsState, ESPMode::ThreadSafe> State = GetSingleState(Ali.FilePath);

		State->LockUser = Ali.LockUserName;

		if (GetUserName() == Ali.LockUserName)
		{
			State->LockState = ELockState::Locked;
		}
		else
		{
			if (Ali.LockUserName.IsEmpty())
			{
				State->LockState = ELockState::NotLocked;
			}
			else
			{
				State->LockState = ELockState::LockedOther;
			}
		}
	}
}

void FUeLfsProvider::UpdateAddedStates(const TArray<FString>& AddedFiles)
{
	for (const FString& FilePath : AddedFiles)
	{
		UE_LOG(LogSourceControl, Warning, TEXT("[UeLfs] Update added file: %s"),
			*FilePath);

		TSharedRef<FUeLfsState, ESPMode::ThreadSafe> State = GetSingleState(FilePath);
		State->WorkingCopyState = EWorkingCopyState::Added;
	}
}

void FUeLfsProvider::UpdateDeletedFiles(const TArray<FString>& DeletedFiles)
{
	for (const FString& FilePath : DeletedFiles)
	{
		UE_LOG(LogSourceControl, Warning, TEXT("[UeLfs] Update deleted file: %s"),
			*FilePath);

		TSharedRef<FUeLfsState, ESPMode::ThreadSafe> State = GetSingleState(FilePath);
		State->WorkingCopyState = EWorkingCopyState::Deleted;
	}
}

void FUeLfsProvider::UpdateModifiedStates(const TArray<FString>& ModifiedFiles)
{
	for (const FString& FilePath : ModifiedFiles)
	{
		UE_LOG(LogSourceControl, Warning, TEXT("[UeLfs] Update modified file: %s"),
			*FilePath);

		TSharedRef<FUeLfsState, ESPMode::ThreadSafe> State = GetSingleState(FilePath);
		State->WorkingCopyState = EWorkingCopyState::Modified;
	}
}

void FUeLfsProvider::ReleaseAllMyLocks(const FString& MyUserName)
{
	for (const auto& Elem : StateCache)
	{
		TSharedRef<FUeLfsState, ESPMode::ThreadSafe> State = Elem.Value;
		if (State->LockUser == MyUserName)
		{
			State->LockUser.Empty();
			State->LockState = ELockState::NotLocked;
		}
	}
}

TArray<FLfsLockItem> FUeLfsProvider::MakeLockItems(const FString& RepoRootPath,
	const TArray<FString>& FilePaths)
{
	TArray<FLfsLockItem> Items;

	for (const FString& FilePath : FilePaths)
	{
		TSharedRef<FUeLfsState, ESPMode::ThreadSafe> State = GetSingleState(FilePath);

		FLfsLockItem Item;
		Item.LocalFilePath = FilePath;
		Item.GitFilePath = State->GitFilePath;

		if (Item.LastHash.IsEmpty())
		{
			UeLfsUtils::GetLastCommitHash(State->LocalFileName, RepoRootPath, State->LastCommitHash);
		}
		Item.LastHash = State->LastCommitHash;

		Items.Add(Item);
	}

	return Items;
}

void FUeLfsProvider::GetLockedFiles(TArray<FLfsLockItem>& OutItems, const FString& UserName)
{
	for (const auto& Elem : StateCache)
	{
		TSharedRef<FUeLfsState, ESPMode::ThreadSafe> State = Elem.Value;
		if (State->LockUser == UserName)
		{
			FLfsLockItem Ali;
			Ali.LocalFilePath = State->LocalFileName;
			Ali.GitFilePath = State->GitFilePath;
			Ali.LastHash = State->LastCommitHash;

			OutItems.Emplace(Ali);
		}
	}
}

TSharedPtr<class IUeLfsWorker, ESPMode::ThreadSafe> FUeLfsProvider::CreateWorker(
	const FName& InOperationName) const
{
	const FGetUeLfsWorker* Operation = WorkersMap.Find(InOperationName);
	if (Operation != NULL)
	{
		return Operation->Execute();
	}

	return NULL;
}

ECommandResult::Type FUeLfsProvider::ExecuteSynchronousCommand(
	class FUeLfsCommand& InCommand,
	const FText& Task,
	bool bSuppressResponseMsg)
{
	ECommandResult::Type Result = ECommandResult::Failed;

	// Display the progress dialog if a string was provided
	{
		FScopedSourceControlProgress Progress(Task);

		// Perform the command asynchronously
		IssueCommand(InCommand, false);

		double LastTime = FPlatformTime::Seconds();
		while (!InCommand.bExecuteProcessed)
		{
			const double AppTime = FPlatformTime::Seconds();
			UeLfsUtils::TickHttp(AppTime - LastTime);
			LastTime = AppTime;

			// Tick the command queue and update progress.
			Tick();

			Progress.Tick();

			// Sleep for a bit so we don't busy-wait so much.
			FPlatformProcess::Sleep(0.01f);
		}

		// always do one more Tick() to make sure the command queue is cleaned up.
		Tick();

		if (InCommand.bCommandSuccessful)
		{
			Result = ECommandResult::Succeeded;
		}
	}


	// If the command failed, inform the user that they need to try again
	if (Result != ECommandResult::Succeeded && !bSuppressResponseMsg)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("UeLfs_ServerUnresponsive", "UeLfs server is not responding. Please check your connection and try again."));
	}

	// Delete the command now
	check(!InCommand.bAutoDelete);

	// ensure commands that are not auto deleted do not end up in the command queue
	if (CommandQueue.Contains(&InCommand))
	{
		CommandQueue.Remove(&InCommand);
	}

	delete &InCommand;

	return Result;
}

ECommandResult::Type FUeLfsProvider::IssueCommand(class FUeLfsCommand& InCommand,
	const bool bSynchronous)
{
	if (!bSynchronous && GThreadPool != NULL)
	{
		// Queue this to our worker thread(s) for resolving.
		// When asynchronous, any callback gets called from Tick().
		GThreadPool->AddQueuedWork(&InCommand);
		CommandQueue.Add(&InCommand);
		return ECommandResult::Succeeded;
	}
	else
	{
		InCommand.bCommandSuccessful = InCommand.DoWork();
		InCommand.Worker->UpdateStates();
		OutputCommandMessages(InCommand);

		return InCommand.ReturnResults();
	}
}

void FUeLfsProvider::OutputCommandMessages(const class FUeLfsCommand& InCommand) const
{
	FMessageLog SourceControlLog("SourceControl");

	for (int32 ErrorIndex = 0; ErrorIndex < InCommand.ErrorMessages.Num(); ++ErrorIndex)
	{
		SourceControlLog.Error(FText::FromString(InCommand.ErrorMessages[ErrorIndex]));
	}

	for (int32 InfoIndex = 0; InfoIndex < InCommand.InfoMessages.Num(); ++InfoIndex)
	{
		SourceControlLog.Info(FText::FromString(InCommand.InfoMessages[InfoIndex]));
	}
}

TSharedRef<FUeLfsState, ESPMode::ThreadSafe> FUeLfsProvider::GetSingleState(
	const FString& FilePath)
{
	TSharedRef<FUeLfsState, ESPMode::ThreadSafe>* State = StateCache.Find(FilePath);
	if (State != NULL)
	{
		return *State;
	}

	// Create a new state for this file.
	TSharedRef<FUeLfsState, ESPMode::ThreadSafe> NewState =
		MakeShareable(new FUeLfsState(FilePath));

	if (UeLfsUtils::CheckFilename(FilePath))
	{
		// Set git path.
		FUeLfsModule& UeLfs = FModuleManager::GetModuleChecked<FUeLfsModule>("UeLfs");
		const FString& RepoRootPath = UeLfs.AccessSettings().GetRepoRootPath();
		NewState->GitFilePath = FilePath;
		FPaths::MakePathRelativeTo(NewState->GitFilePath, *RepoRootPath);

		// Set last commit hash.
		//UeLfsUtils::GetLastCommitHash(NewState->LocalFileName, RepoRootPath, NewState->LastCommitHash);
	}
	else
	{
		NewState->WorkingCopyState = EWorkingCopyState::NotControlled;
	}

	StateCache.Add(FilePath, NewState);

	return NewState;
}

#undef LOCTEXT_NAMESPACE // "UeLfs"
