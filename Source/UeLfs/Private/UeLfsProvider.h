// ----------------------------------------------------------------------------
// © 2022 Eungsik Yoon <yoon.eungsik@gmail.com>
// ----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "ISourceControlState.h"
#include "UeLfsState.h"
#include "ISourceControlOperation.h"
#include "ISourceControlProvider.h"
#include "IUeLfsWorker.h"
#include "UeLfsUtils.h"

DECLARE_DELEGATE_RetVal(FUeLfsWorkerRef, FGetUeLfsWorker)

class FUeLfsProvider : public ISourceControlProvider
{
public:
	/** Constructor */
	FUeLfsProvider()
	{
	}

	/* ISourceControlProvider implementation */
	virtual void Init(bool bForceConnection = true) override;
	virtual void Close() override;
	virtual FText GetStatusText() const override;
	virtual bool IsEnabled() const override;
	virtual bool IsAvailable() const override;
	virtual const FName& GetName(void) const override;
	virtual bool QueryStateBranchConfig(const FString& ConfigSrc, const FString& ConfigDest) override;
	virtual void RegisterStateBranches(const TArray<FString>& BranchNames, const FString& ContentRoot) override;
	virtual int32 GetStateBranchIndex(const FString& BranchName) const override { return INDEX_NONE; }
	virtual ECommandResult::Type GetState( const TArray<FString>& InFiles, TArray< TSharedRef<ISourceControlState, ESPMode::ThreadSafe> >& OutState, EStateCacheUsage::Type InStateCacheUsage ) override;
	virtual TArray<FSourceControlStateRef> GetCachedStateByPredicate(TFunctionRef<bool(const FSourceControlStateRef&)> Predicate) const override;
	virtual FDelegateHandle RegisterSourceControlStateChanged_Handle( const FSourceControlStateChanged::FDelegate& SourceControlStateChanged ) override;
	virtual void UnregisterSourceControlStateChanged_Handle( FDelegateHandle Handle ) override;
	virtual ECommandResult::Type Execute(const TSharedRef<ISourceControlOperation,
		ESPMode::ThreadSafe>& InOperation,
		const TArray<FString>& InFiles,
		EConcurrency::Type InConcurrency = EConcurrency::Synchronous,
		const FSourceControlOperationComplete& InOperationCompleteDelegate = FSourceControlOperationComplete()) override;
	virtual bool CanCancelOperation( const TSharedRef<ISourceControlOperation, ESPMode::ThreadSafe>& InOperation ) const override;
	virtual void CancelOperation( const TSharedRef<ISourceControlOperation, ESPMode::ThreadSafe>& InOperation ) override;
	virtual bool UsesLocalReadOnlyState() const override;
	virtual bool UsesChangelists() const override;
	virtual bool UsesCheckout() const override;
	virtual void Tick() override;
	virtual TArray< TSharedRef<class ISourceControlLabel> > GetLabels( const FString& InMatchingSpec ) const override;

#if SOURCE_CONTROL_WITH_SLATE
	virtual TSharedRef<class SWidget> MakeSettingsWidget() const override;
#endif

	const FString& GetUserName() const;

	/**
	 * Register a worker with the provider.
	 * This is used internally so the provider can maintain a map of all available operations.
	 */
	void RegisterWorker(const FName& InName, const FGetUeLfsWorker& InDelegate);

	// Update locked state.
	void UpdateLockedStates(const TArray<FLfsLockInfo>& LockInfos);

	// Update added state.
	void UpdateAddedStates(const TArray<FString>& AddedFiles);

	// Update deleted states.
	void UpdateDeletedFiles(const TArray<FString>& DeletedFiles);

	// Update modified states.
	void UpdateModifiedStates(const TArray<FString>& ModifiedFiles);

	// Release all "my" locks.
	void ReleaseAllMyLocks(const FString& MyUserName);

	// Build FLfsLockItem array from file path string array.
	TArray<FLfsLockItem> MakeLockItems(const FString& RepoRootPath, const TArray<FString>& FilePaths);

	// Get files locked by 'UserName'.
	void GetLockedFiles(TArray<FLfsLockItem>& OutLockItems, const FString& UserName);

private:

	// Helper function for Execute().
	TSharedPtr<class IUeLfsWorker, ESPMode::ThreadSafe> CreateWorker(
		const FName& InOperationName) const;

	// Run a command synchronously.
	ECommandResult::Type ExecuteSynchronousCommand(class FUeLfsCommand& InCommand,
		const FText& Task,
		bool bSuppressResponseMsg);

	// Run a command synchronously or asynchronously.
	ECommandResult::Type IssueCommand(class FUeLfsCommand& InCommand,
		const bool bSynchronous);

	// Output any messages this command holds
	void OutputCommandMessages(const class FUeLfsCommand& InCommand) const;

	// Get single state.
	TSharedRef<FUeLfsState, ESPMode::ThreadSafe> GetSingleState(
		const FString& FilePath);

private:

	/** State cache */
	TMap<FString, TSharedRef<class FUeLfsState, ESPMode::ThreadSafe>> StateCache;

	/** The currently registered source control operations */
	TMap<FName, FGetUeLfsWorker> WorkersMap;

	/** Queue for commands given by the main thread */
	TArray<FUeLfsCommand*> CommandQueue;

	/** For notifying when the source control states in the cache have changed */
	FSourceControlStateChanged OnSourceControlStateChanged;

};
