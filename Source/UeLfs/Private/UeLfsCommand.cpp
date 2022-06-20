// ----------------------------------------------------------------------------
// © 2022 Eungsik Yoon <yoon.eungsik@gmail.com>
// ----------------------------------------------------------------------------

#include "UeLfsCommand.h"
#include "Modules/ModuleManager.h"
#include "UeLfsModule.h"
#include "SUeLfsSettings.h"

FUeLfsCommand::FUeLfsCommand(
	const TSharedRef<class ISourceControlOperation, ESPMode::ThreadSafe>& InOperation,
	const TSharedRef<class IUeLfsWorker, ESPMode::ThreadSafe>& InWorker,
	const FSourceControlOperationComplete& InOperationCompleteDelegate)
	: Operation(InOperation)
	, Worker(InWorker)
	, OperationCompleteDelegate(InOperationCompleteDelegate)
	, bExecuteProcessed(0)
	, bCommandSuccessful(false)
	, bAutoDelete(true)
	, Concurrency(EConcurrency::Synchronous)
{
	// grab the providers settings here, so we don't access them once the worker thread is launched
	check(IsInGameThread());

	FUeLfsModule& UeLfs = FModuleManager::LoadModuleChecked<FUeLfsModule>( "UeLfs" );
	FUeLfsProvider& Provider = UeLfs.GetProvider();
	ServerUrl = UeLfs.AccessSettings().GetServerUrl();
	UserName = UeLfs.AccessSettings().GetUserName();
}

bool FUeLfsCommand::DoWork()
{
	bCommandSuccessful = Worker->Execute(*this);
	FPlatformAtomics::InterlockedExchange(&bExecuteProcessed, 1);

	return bCommandSuccessful;
}

void FUeLfsCommand::Abandon()
{
	FPlatformAtomics::InterlockedExchange(&bExecuteProcessed, 1);
}

void FUeLfsCommand::DoThreadedWork()
{
	Concurrency = EConcurrency::Asynchronous;
	DoWork();
}

ECommandResult::Type FUeLfsCommand::ReturnResults()
{
	// Save any messages that have accumulated
	for (FString& String : InfoMessages)
	{
		Operation->AddInfoMessge(FText::FromString(String));
	}
	for (FString& String : ErrorMessages)
	{
		Operation->AddErrorMessge(FText::FromString(String));
	}

	// run the completion delegate if we have one bound
	ECommandResult::Type Result = bCommandSuccessful ? ECommandResult::Succeeded : ECommandResult::Failed;
	OperationCompleteDelegate.ExecuteIfBound(Operation, Result);

	return Result;
}
