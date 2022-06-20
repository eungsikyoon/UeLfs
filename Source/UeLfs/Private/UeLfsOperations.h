// ----------------------------------------------------------------------------
// © 2022 Eungsik Yoon <yoon.eungsik@gmail.com>
// ----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "UeLfsState.h"
#include "IUeLfsWorker.h"
#include "UeLfsUtils.h"

//-----------------------------------------------------------------------------
class FUeLfsWorkerConnect : public IUeLfsWorker
{
public:
	virtual ~FUeLfsWorkerConnect() {}

	// IUeLfsWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUeLfsCommand& InCommand) override;
	virtual bool UpdateStates() const override;

private:
	TArray<FLfsLockInfo> LockInfos;
};

//-----------------------------------------------------------------------------
class FUeLfsWorkerUpdateStatus : public IUeLfsWorker
{
public:
	virtual ~FUeLfsWorkerUpdateStatus() {}

	// IUeLfsWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUeLfsCommand& InCommand) override;
	virtual bool UpdateStates() const override;

private:
	TArray<FLfsLockInfo> LockInfos;
	TArray<FString> AddedFiles;
};

//-----------------------------------------------------------------------------
class FUeLfsWorkerCheckOut: public IUeLfsWorker
{
public:
	virtual ~FUeLfsWorkerCheckOut() {}

	// IUeLfsWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUeLfsCommand& InCommand) override;
	virtual bool UpdateStates() const override;

private:
	TArray<FLfsLockInfo> LockInfos;
	TArray<FString> ModifiedFiles;
};

//-----------------------------------------------------------------------------
class FUeLfsWorkerCheckIn : public IUeLfsWorker
{
public:
	virtual ~FUeLfsWorkerCheckIn() {}

	// IUeLfsWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUeLfsCommand& InCommand) override;
	virtual bool UpdateStates() const override;

private:
	TArray<FLfsLockInfo> LockInfos;
};

//-----------------------------------------------------------------------------
class FUeLfsWorkerDelete : public IUeLfsWorker
{
public:
	virtual ~FUeLfsWorkerDelete() {}

	// IUeLfsWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUeLfsCommand& InCommand) override;
	virtual bool UpdateStates() const override;

private:
	TArray<FLfsLockInfo> LockInfos;
	TArray<FString> DeletedFiles;
};

//-----------------------------------------------------------------------------
class FUeLfsWorkerRevert : public IUeLfsWorker
{
public:
	virtual ~FUeLfsWorkerRevert() {}

	// IUeLfsWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUeLfsCommand& InCommand) override;
	virtual bool UpdateStates() const override;
};

//-----------------------------------------------------------------------------
class FUeLfsWorkerCopy : public IUeLfsWorker
{
public:
	virtual ~FUeLfsWorkerCopy() {}

	// IUeLfsWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUeLfsCommand& InCommand) override;
	virtual bool UpdateStates() const override;
};

//-----------------------------------------------------------------------------
class FUeLfsWorkerMarkForAdd : public IUeLfsWorker
{
public:
	virtual ~FUeLfsWorkerMarkForAdd() {}

	// IUeLfsWorker interface
	virtual FName GetName() const override;
	virtual bool Execute(class FUeLfsCommand& InCommand) override;
	virtual bool UpdateStates() const override;
};
