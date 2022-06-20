// ----------------------------------------------------------------------------
// © 2022 Eungsik Yoon <yoon.eungsik@gmail.com>
// ----------------------------------------------------------------------------

#include "UeLfsUtils.h"
#include "UeLfsSettings.h"
#include "ISourceControlModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HttpModule.h"
#include "HttpManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformProcess.h"
#include "HAL/FileManager.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Paths.h"
#include "Misc/MessageDialog.h"

bool UeLfsUtils::CheckFilename(const FString& FileName)
{
	if (FileName.Contains(TEXT("...")) ||
		FileName.Contains(TEXT("*")) ||
		FileName.Contains(TEXT("?")))
	{
		UE_LOG(LogSourceControl, Warning,
			TEXT("Filename '%s' with wildcards is not supported by UeLfs"), *FileName);
		return false;
	}

	if (!FileName.EndsWith(TEXT(".uasset")) && !FileName.EndsWith(TEXT(".umap")))
	{
		UE_LOG(LogSourceControl, Warning,
			TEXT("File '%s' is not supported by UeLfs"), *FileName);
		return false;
	}

	return true;
}

bool UeLfsUtils::CheckFilenames(const TArray<FString>& FileNames)
{
	bool bResult = true;

	for(auto Iter(FileNames.CreateConstIterator()); Iter; Iter++)
	{
		bResult &= CheckFilename(*Iter);
	}

	return bResult;
}

TArray<FString> UeLfsUtils::ToGitFilePaths(const TArray<FString>& AbsFilePaths,
	const FString& RepoRootPath)
{
	TArray<FString> GitFilePaths;

	for (const FString& AbsFilePath : AbsFilePaths)
	{
		FString GitFilePath = AbsFilePath;
		FPaths::MakePathRelativeTo(GitFilePath, *RepoRootPath);
		GitFilePaths.Add(GitFilePath);
	}

	return GitFilePaths;
}

FString UeLfsUtils::FindGitBinaryPath()
{
	FString BinPath;

#if PLATFORM_WINDOWS // We only support Windows at the moment.
	int32 ReturnCode;
	FString StdOut;
	FString StdErr;
	FPlatformProcess::ExecProcess(TEXT("where"), TEXT("git"), &ReturnCode, &StdOut, &StdErr);

	//UE_LOG(LogSourceControl, Warning, TEXT("[TEMP] FindGitBinaryPath ... ReturnCode: %d"), ReturnCode);
	//UE_LOG(LogSourceControl, Warning, TEXT("[TEMP] FindGitBinaryPath ... StdOut: %s"), *StdOut);
	//UE_LOG(LogSourceControl, Warning, TEXT("[TEMP] FindGitBinaryPath ... StdErr: %s"), *StdErr);

	if (ReturnCode == 0)
	{
		BinPath = StdOut;
	}
	else
	{
		UE_LOG(LogSourceControl, Warning, TEXT("git binary not found! - %s"), *StdOut);
	}

#endif // PLATFORM_WINDOWS

	return BinPath;
}

bool UeLfsUtils::FindRepoRootPath(const FString& ProjectDirAbs, FString& OutRepoRootPath)
{
	FString PathToGitSubdirectory;
	OutRepoRootPath = ProjectDirAbs;

	auto TrimTrailing = [](FString& Str, const TCHAR Char)
	{
		int32 Len = Str.Len();
		while (Len && Str[Len - 1] == Char)
		{
			Str = Str.LeftChop(1);
			Len = Str.Len();
		}
	};

	TrimTrailing(OutRepoRootPath, '\\');
	TrimTrailing(OutRepoRootPath, '/');

	bool bFound = false;
	while (!bFound && !OutRepoRootPath.IsEmpty())
	{
		// Look for the ".git" subdirectory (or file) present at the root of every Git repository
		PathToGitSubdirectory = OutRepoRootPath / TEXT(".git");
		bFound = IFileManager::Get().DirectoryExists(*PathToGitSubdirectory) || IFileManager::Get().FileExists(*PathToGitSubdirectory);
		if (!bFound)
		{
			int32 LastSlashIndex;
			if (OutRepoRootPath.FindLastChar('/', LastSlashIndex))
			{
				OutRepoRootPath = OutRepoRootPath.Left(LastSlashIndex);
			}
			else
			{
				OutRepoRootPath.Empty();
			}
		}
	}

	if (!bFound)
	{
		OutRepoRootPath = ProjectDirAbs; // If not found, return the provided dir as best possible root.
	}
	else
	{
		OutRepoRootPath += '/';
	}

	return bFound;
}

FString UeLfsUtils::GetGitUserName()
{
	FString ArgStr = FString::Printf(TEXT("config user.name"));

	int32 ReturnCode;
	FString StdOut;
	FString StdErr;
	FPlatformProcess::ExecProcess(TEXT("git"), *ArgStr, &ReturnCode, &StdOut, &StdErr);

	if (ReturnCode == 0)
	{
		StdOut.TrimStartAndEndInline();
		return StdOut;
	}

	UE_LOG(LogSourceControl, Error, TEXT("Failed to get git user name! - %s"), *StdErr);
	return TEXT("");
}

bool UeLfsUtils::GetLastCommitHash(const FString& FilePathAbs, const FString& RepoRootPath, FString& OutHash)
{
	FString GitBranchName = UeLfsUtils::GetGitBranchName(RepoRootPath);

	FString ArgStr = FString::Printf(TEXT("-C %s log %s -n 1 --pretty=format:%%H %s"),
		*RepoRootPath,
		*GitBranchName,
		*FilePathAbs);

	int32 ReturnCode;
	FString StdOut;
	FString StdErr;
	FPlatformProcess::ExecProcess(TEXT("git"), *ArgStr, &ReturnCode, &StdOut, &StdErr);

	if (ReturnCode == 0)
	{
		OutHash = StdOut;
		return true;
	}

	UE_LOG(LogSourceControl, Error, TEXT("Failed to get last commit hash! - %s"), *StdErr);
	return false;
}

bool UeLfsUtils::GetAddedFiles(const TArray<FString>& FilePaths, const FString& RepoRootPath,
	TArray<FString>& OutAddedFiles)
{
	FString ArgStr = FString::Printf(TEXT("-C %s ls-files"),
		*RepoRootPath);

	TArray<FString> GitFilePaths = UeLfsUtils::ToGitFilePaths(FilePaths, RepoRootPath);
	for (const FString& GitFilePath : GitFilePaths)
	{
		ArgStr += " " + GitFilePath;
	}

	int32 ReturnCode;
	FString StdOut;
	FString StdErr;
	FPlatformProcess::ExecProcess(TEXT("git"), *ArgStr, &ReturnCode, &StdOut, &StdErr);

	if (ReturnCode == 0)
	{
		TArray<FString> OutputArray;
		int32 numLines = StdOut.ParseIntoArrayLines(OutputArray);
		if (numLines == GitFilePaths.Num())
		{
			// No missing files.
			return true;
		}

		for (int i = 0, j = 0; i < GitFilePaths.Num(); ++i)
		{
			if (j < OutputArray.Num() && GitFilePaths[i] == OutputArray[j])
			{
				++j;
			}
			else
			{
				OutAddedFiles.Add(FilePaths[i]);
			}
		}

		return true;
	}

	UE_LOG(LogSourceControl, Error, TEXT("Failed to get added files! - %s"), *StdErr);
	return false;
}

void UeLfsUtils::TickHttp(float deltaSeconds)
{
	FHttpModule::Get().GetHttpManager().Tick(deltaSeconds);
}

FString UeLfsUtils::GetGitBranchName(const FString& RepoRootPath)
{
	FString ArgStr = FString::Printf(TEXT("-C %s rev-parse --abbrev-ref HEAD"), *RepoRootPath);

	int32 ReturnCode;
	FString StdOut;
	FString StdErr;
	FPlatformProcess::ExecProcess(TEXT("git"), *ArgStr, &ReturnCode, &StdOut, &StdErr);

	if (ReturnCode == 0)
	{
		StdOut.TrimStartAndEndInline();
		return StdOut;
	}

	UE_LOG(LogSourceControl, Error, TEXT("Failed to get git branch name! - %s"), *StdErr);
	return TEXT("");
}

FUeLfsHttp::FUeLfsHttp()
	: bLoggedIn(false)
{
}

void FUeLfsHttp::Configure(const FUeLfsSettings& Settings)
{
	ServerUrl = Settings.GetServerUrl();
	UserName = Settings.GetUserName();
	RepoRootPath = Settings.GetRepoRootPath();
}

bool FUeLfsHttp::ReqLogin(TArray<FString>& OutGitPaths)
{
	// Set API URL.
	FString ApiUrl = FString::Printf(TEXT("%s/unsafeLogin"), *ServerUrl);

	// Build request body.
	TSharedPtr<FJsonObject> ReqObj = MakeShareable(new FJsonObject);
	ReqObj->SetStringField(TEXT("user"), UserName);

	FString GitBranchName = UeLfsUtils::GetGitBranchName(RepoRootPath);
	ReqObj->SetStringField(TEXT("branch"), GitBranchName);

	FString ReqStr;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&ReqStr);
	FJsonSerializer::Serialize(ReqObj.ToSharedRef(), Writer);

	// Build HTTP request.
	auto HttpReq = FHttpModule::Get().CreateRequest();
	HttpReq->SetHeader(TEXT("Content-Type"), TEXT("application/json; charset=utf-8"));
	HttpReq->SetURL(ApiUrl);
	HttpReq->SetVerb(TEXT("POST"));
	HttpReq->SetContentAsString(ReqStr);
	HttpReq->ProcessRequest();

	// Synchronize the invocation of this function.
	while (HttpReq->GetStatus() == EHttpRequestStatus::Processing)
	{
		FPlatformProcess::Sleep(0.2f);
	}

	const FHttpResponsePtr Resp = HttpReq->GetResponse();
	if (!Resp.IsValid())
	{
		UE_LOG(LogSourceControl, Error, TEXT("[/dumbLogin] No response from server!"));
		return false;
	}

	const FString& RespBodyStr = Resp->GetContentAsString();
	UE_LOG(LogSourceControl, Warning,
		TEXT("[/dumbLogin] resp: %d, content: %s"),
		(int32)Resp->GetResponseCode(),
		*RespBodyStr);

	// Parse results.
	TSharedPtr<FJsonObject> RespObj;
	TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create(RespBodyStr);
	if (!FJsonSerializer::Deserialize(Reader, RespObj))
	{
		UE_LOG(LogSourceControl, Error,
			TEXT("[/dumbLogin] Failed to parse response body! - %s"),
			*RespBodyStr);
		return false;
	}

	// Check if ok.
	bool bOk = RespObj->GetBoolField(TEXT("ok"));
	if (!bOk)
	{
		const FString& ErrorMsg = RespObj->GetStringField(TEXT("msg"));
		UE_LOG(LogSourceControl, Error,
			TEXT("[/dumbLogin] Error - %s"),
			*ErrorMsg);
		return false;
	}

	// Read locked files.
	if (RespObj->HasField(TEXT("lockedFiles")))
	{
		const TArray<TSharedPtr<FJsonValue>>& LockedFiles = RespObj->GetArrayField(TEXT("lockedFiles"));
		for (int32 i = 0; i < LockedFiles.Num(); ++i)
		{
			OutGitPaths.Emplace(LockedFiles[i]->AsString());
		}
	}

	bLoggedIn = true;

	return true;
}

bool FUeLfsHttp::ReqGetLockStates(const TArray<FString>& FilePaths,
	TArray<FLfsLockInfo>& OutLockInfos)
{
	// Set API URL.
	FString ApiUrl = FString::Printf(TEXT("%s/getLockStates"), *ServerUrl);

	// Build request body.
	TSharedPtr<FJsonObject> ReqObj = MakeShareable(new FJsonObject);
	ReqObj->SetStringField(TEXT("user"), UserName);

	FString GitBranchName = UeLfsUtils::GetGitBranchName(RepoRootPath);
	ReqObj->SetStringField(TEXT("branch"), GitBranchName);

	TArray<TSharedPtr<FJsonValue>> JFiles;
	for (const FString& FilePath : FilePaths)
	{
		FString GitFilePath = FilePath;
		FPaths::MakePathRelativeTo(GitFilePath, *RepoRootPath);
		JFiles.Add(MakeShareable(new FJsonValueString(GitFilePath)));
	}

	ReqObj->SetArrayField(TEXT("files"), JFiles);

	FString ReqStr;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&ReqStr);
	FJsonSerializer::Serialize(ReqObj.ToSharedRef(), Writer);

	// Build HTTP request.
	auto HttpReq = FHttpModule::Get().CreateRequest();
	HttpReq->SetHeader(TEXT("Content-Type"), TEXT("application/json; charset=utf-8"));
	HttpReq->SetURL(ApiUrl);
	HttpReq->SetVerb(TEXT("POST"));
	HttpReq->SetContentAsString(ReqStr);
	HttpReq->ProcessRequest();

	// Synchronize the invocation of this function.
	while (HttpReq->GetStatus() == EHttpRequestStatus::Processing)
	{
		FPlatformProcess::Sleep(0.2f);
	}

	const FHttpResponsePtr Resp = HttpReq->GetResponse();
	if (!Resp.IsValid())
	{
		UE_LOG(LogSourceControl, Error, TEXT("[/getLockSates] No response from server!"));
		return false;
	}

	const FString& RespBodyStr = Resp->GetContentAsString();
	UE_LOG(LogSourceControl, Warning,
		TEXT("[/getLockSates] resp: %d, content: %s"),
		(int32)Resp->GetResponseCode(),
		*RespBodyStr);

	// Parse results.
	TSharedPtr<FJsonObject> RespObj;
	TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create(RespBodyStr);
	if (!FJsonSerializer::Deserialize(Reader, RespObj))
	{
		UE_LOG(LogSourceControl, Error,
			TEXT("[/getLockSates] Failed to parse response body! - %s"),
			*RespBodyStr);
		return false;
	}

	// Check if ok.
	bool bOk = RespObj->GetBoolField(TEXT("ok"));
	if (!bOk)
	{
		const FString& ErrorMsg = RespObj->GetStringField(TEXT("msg"));
		UE_LOG(LogSourceControl, Error,
			TEXT("[/getLockSates] Error - %s"),
			*ErrorMsg);
		return false;
	}

	// Read lock states.
	const TArray<TSharedPtr<FJsonValue>>& LockStates = RespObj->GetArrayField(TEXT("lockStates"));
	if (LockStates.Num() != FilePaths.Num())
	{
		UE_LOG(LogSourceControl, Error,
			TEXT("[/getLockSates] Invalid number of lock states! - %d vs %d"),
			LockStates.Num(), FilePaths.Num());
		return false;
	}

	for (int32 i = 0; i < LockStates.Num(); ++i)
	{
		const TSharedPtr<FJsonObject>& LockStateObj = LockStates[i]->AsObject();

		FLfsLockInfo Ali;
		Ali.FilePath = FilePaths[i];

		if (LockStateObj->HasField(TEXT("user")))
		{
			Ali.LockUserName = LockStateObj->GetStringField(TEXT("user"));
		}

		OutLockInfos.Add(Ali);
	}

	return true;
}

bool FUeLfsHttp::ReqLockFiles(const TArray<FLfsLockItem>& LockItems,
	TArray<FLfsLockInfo>& OutLockInfos)
{
	// Set API URL.
	FString ApiUrl = FString::Printf(TEXT("%s/lockFiles"), *ServerUrl);

	// Build request body.
	TSharedPtr<FJsonObject> ReqObj = MakeShareable(new FJsonObject);
	ReqObj->SetStringField(TEXT("user"), UserName);

	FString GitBranchName = UeLfsUtils::GetGitBranchName(RepoRootPath);
	ReqObj->SetStringField(TEXT("branch"), GitBranchName);

	TArray<TSharedPtr<FJsonValue>> JFiles;
	for (const FLfsLockItem& LockItem : LockItems)
	{
		TSharedPtr<FJsonObject> FileAndHashObj = MakeShareable(new FJsonObject);
		FileAndHashObj->SetStringField(TEXT("hash"), LockItem.LastHash);
		FileAndHashObj->SetStringField(TEXT("path"), LockItem.GitFilePath);

		JFiles.Add(MakeShareable(new FJsonValueObject(FileAndHashObj)));
	}

	ReqObj->SetArrayField(TEXT("files"), JFiles);

	FString ReqStr;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&ReqStr);
	FJsonSerializer::Serialize(ReqObj.ToSharedRef(), Writer);

	// Build HTTP request.
	auto HttpReq = FHttpModule::Get().CreateRequest();
	HttpReq->SetHeader(TEXT("Content-Type"), TEXT("application/json; charset=utf-8"));
	HttpReq->SetURL(ApiUrl);
	HttpReq->SetVerb(TEXT("POST"));
	HttpReq->SetContentAsString(ReqStr);
	HttpReq->ProcessRequest();

	// Synchronize the invocation of this function.
	while (HttpReq->GetStatus() == EHttpRequestStatus::Processing)
	{
		FPlatformProcess::Sleep(0.2f);
	}

	const FHttpResponsePtr Resp = HttpReq->GetResponse();
	if (!Resp.IsValid())
	{
		UE_LOG(LogSourceControl, Error, TEXT("[/lockFiles] No response from server!"));
		return false;
	}

	const FString& RespBodyStr = Resp->GetContentAsString();
	UE_LOG(LogSourceControl, Warning,
		TEXT("[/lockFiles] resp: %d, content: %s"),
		(int32)Resp->GetResponseCode(),
		*RespBodyStr);

	// Parse results.
	TSharedPtr<FJsonObject> RespObj;
	TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create(RespBodyStr);
	if (!FJsonSerializer::Deserialize(Reader, RespObj))
	{
		UE_LOG(LogSourceControl, Error,
			TEXT("[/lockFiles] Failed to parse response body! - %s"),
			*RespBodyStr);
		return false;
	}

	// Check if ok.
	bool bOk = RespObj->GetBoolField(TEXT("ok"));
	if (!bOk)
	{
		const FString& ErrorMsg = RespObj->GetStringField(TEXT("msg"));
		UE_LOG(LogSourceControl, Error,
			TEXT("[/lockFiles] Error - %s"),
			*ErrorMsg);
		return false;
	}

	// If successful, all requested files have been locked.
	for (int32 i = 0; i < LockItems.Num(); ++i)
	{
		FLfsLockInfo Ali;
		Ali.FilePath = LockItems[i].LocalFilePath;
		Ali.LockUserName = UserName;

		OutLockInfos.Add(Ali);
	}

	return true;
}

bool FUeLfsHttp::ReqUnlockFiles(const TArray<FLfsLockItem>& LockItems,
	TArray<FLfsLockInfo>& OutLockInfos)
{
	// Set API URL.
	FString ApiUrl = FString::Printf(TEXT("%s/unlockFiles"), *ServerUrl);

	// Build request body.
	TSharedPtr<FJsonObject> ReqObj = MakeShareable(new FJsonObject);
	ReqObj->SetStringField(TEXT("user"), UserName);

	FString GitBranchName = UeLfsUtils::GetGitBranchName(RepoRootPath);
	ReqObj->SetStringField(TEXT("branch"), GitBranchName);

	TArray<TSharedPtr<FJsonValue>> JFiles;
	for (const FLfsLockItem& LockItem : LockItems)
	{
		JFiles.Add(MakeShareable(new FJsonValueString(LockItem.GitFilePath)));
	}

	ReqObj->SetArrayField(TEXT("files"), JFiles);

	FString ReqStr;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&ReqStr);
	FJsonSerializer::Serialize(ReqObj.ToSharedRef(), Writer);

	// Build HTTP request.
	auto HttpReq = FHttpModule::Get().CreateRequest();
	HttpReq->SetHeader(TEXT("Content-Type"), TEXT("application/json; charset=utf-8"));
	HttpReq->SetURL(ApiUrl);
	HttpReq->SetVerb(TEXT("POST"));
	HttpReq->SetContentAsString(ReqStr);
	HttpReq->ProcessRequest();

	// Synchronize the invocation of this function.
	double LastTime = FPlatformTime::Seconds();
	while (HttpReq->GetStatus() == EHttpRequestStatus::Processing)
	{
		double AppTime = FPlatformTime::Seconds();
		FHttpModule::Get().GetHttpManager().Tick(AppTime - LastTime);
		LastTime = AppTime;
		FPlatformProcess::Sleep(0.2f);
	}

	const FHttpResponsePtr Resp = HttpReq->GetResponse();
	if (!Resp.IsValid())
	{
		UE_LOG(LogSourceControl, Error, TEXT("[/unlockFiles] No response from server!"));
		return false;
	}

	const FString& RespBodyStr = Resp->GetContentAsString();
	UE_LOG(LogSourceControl, Warning,
		TEXT("[/unlockFiles] resp: %d, content: %s"),
		(int32)Resp->GetResponseCode(),
		*RespBodyStr);

	// Parse results.
	TSharedPtr<FJsonObject> RespObj;
	TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create(RespBodyStr);
	if (!FJsonSerializer::Deserialize(Reader, RespObj))
	{
		UE_LOG(LogSourceControl, Error,
			TEXT("[/unlockFiles] Failed to parse response body! - %s"),
			*RespBodyStr);
		return false;
	}

	// Check if ok.
	bool bOk = RespObj->GetBoolField(TEXT("ok"));
	if (!bOk)
	{
		const FString& ErrorMsg = RespObj->GetStringField(TEXT("msg"));
		UE_LOG(LogSourceControl, Error,
			TEXT("[/unlockFiles] Error - %s"),
			*ErrorMsg);
		return false;
	}

	// If successful, all requested files have been unlocked.
	for (int32 i = 0; i < LockItems.Num(); ++i)
	{
		FLfsLockInfo Ali;
		Ali.FilePath = LockItems[i].LocalFilePath;

		OutLockInfos.Add(Ali);
	}

	return true;
}

bool FUeLfsHttp::ReqUnlockAll()
{
	// Set API URL.
	FString ApiUrl = FString::Printf(TEXT("%s/unlockAll"), *ServerUrl);

	// Build request body.
	TSharedPtr<FJsonObject> ReqObj = MakeShareable(new FJsonObject);
	ReqObj->SetStringField(TEXT("user"), UserName);

	FString GitBranchName = UeLfsUtils::GetGitBranchName(RepoRootPath);
	ReqObj->SetStringField(TEXT("branch"), GitBranchName);

	FString ReqStr;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&ReqStr);
	FJsonSerializer::Serialize(ReqObj.ToSharedRef(), Writer);

	// Build HTTP request.
	auto HttpReq = FHttpModule::Get().CreateRequest();
	HttpReq->SetHeader(TEXT("Content-Type"), TEXT("application/json; charset=utf-8"));
	HttpReq->SetURL(ApiUrl);
	HttpReq->SetVerb(TEXT("POST"));
	HttpReq->SetContentAsString(ReqStr);
	HttpReq->ProcessRequest();

	// Synchronize the invocation of this function.
	double LastTime = FPlatformTime::Seconds();
	while (HttpReq->GetStatus() == EHttpRequestStatus::Processing)
	{
		double AppTime = FPlatformTime::Seconds();
		FHttpModule::Get().GetHttpManager().Tick(AppTime - LastTime);
		LastTime = AppTime;
		FPlatformProcess::Sleep(0.2f);
	}

	const FHttpResponsePtr Resp = HttpReq->GetResponse();
	if (!Resp.IsValid())
	{
		UE_LOG(LogSourceControl, Error, TEXT("[/unlockAll] No response from server!"));
		return false;
	}

	const FString& RespBodyStr = Resp->GetContentAsString();
	UE_LOG(LogSourceControl, Warning,
		TEXT("[/unlockAll] resp: %d, content: %s"),
		(int32)Resp->GetResponseCode(),
		*RespBodyStr);

	// Parse results.
	TSharedPtr<FJsonObject> RespObj;
	TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create(RespBodyStr);
	if (!FJsonSerializer::Deserialize(Reader, RespObj))
	{
		UE_LOG(LogSourceControl, Error,
			TEXT("[/unlockAll] Failed to parse response body! - %s"),
			*RespBodyStr);
		return false;
	}

	// Check if ok.
	bool bOk = RespObj->GetBoolField(TEXT("ok"));
	if (!bOk)
	{
		const FString& ErrorMsg = RespObj->GetStringField(TEXT("msg"));
		UE_LOG(LogSourceControl, Error,
			TEXT("[/unlockAll] Error - %s"),
			*ErrorMsg);
		return false;
	}

	return true;
}
