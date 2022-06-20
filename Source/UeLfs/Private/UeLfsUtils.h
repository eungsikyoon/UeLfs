// ----------------------------------------------------------------------------
// © 2022 Eungsik Yoon <yoon.eungsik@gmail.com>
// ----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"

struct FLfsLockInfo
{
	FString FilePath;
	FString LockUserName;
};

struct FLfsLockItem
{
	FString LocalFilePath;
	FString GitFilePath;
	FString LastHash;
};

class FUeLfsSettings;

namespace UeLfsUtils
{
	// Check for git-supported file names.
	bool CheckFilename(const FString& FileName);
	bool CheckFilenames(const TArray<FString>& FileNames);

	// Convert abs file paths to git file paths.
	TArray<FString> ToGitFilePaths(const TArray<FString>& AbsFilePaths, const FString& RepoRootPath);

	// Find git binary path.
	FString FindGitBinaryPath();

	// Find git repository root path.
	bool FindRepoRootPath(const FString& ProjectDirAbs, FString& OutRepoRootPath);

	// Get git user name.
	FString GetGitUserName();

	// Get last commit has for the file.
	bool GetLastCommitHash(const FString& FilePathAbs, const FString& RepoRootPath, FString& OutHash);

	// Gather newly added files among the files.
	bool GetAddedFiles(const TArray<FString>& FilePaths, const FString& RepoRootPath,
		TArray<FString>& OutAddedFiles);

	// Manually tick http module.
	void TickHttp(float deltaSeconds);

	// Get git branch name.
	FString GetGitBranchName(const FString& RepoRootPath);

}; // namespace UeLfsUtils

class FUeLfsHttp
{
public:
	FUeLfsHttp();

	void Configure(const FUeLfsSettings& Settings);
	bool IsLoggedIn() const { return bLoggedIn; }

public:

	bool ReqLogin(TArray<FString>& OutGitPaths);
	bool ReqGetLockStates(const TArray<FString>& FilePaths, TArray<FLfsLockInfo>& OutLockInfos);
	bool ReqLockFiles(const TArray<FLfsLockItem>& LockReqObjs,
		TArray<FLfsLockInfo>& OutLockInfos);
	bool ReqUnlockFiles(const TArray<FLfsLockItem>& LockReqObjs,
		TArray<FLfsLockInfo>& OutLockInfos);
	bool ReqUnlockAll();

private:
	FString ServerUrl;
	FString UserName;
	FString RepoRootPath;

	bool bLoggedIn;
};
