// ----------------------------------------------------------------------------
// © 2022 Eungsik Yoon <yoon.eungsik@gmail.com>
// ----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"

class FUeLfsSettings
{
public:
	const FString& GetServerUrl() const;
	void SetServerUrl(const FString& InString);

	const FString& GetUserName() const;
	void SetUserName(const FString& InString);

	const FString& GetGitBinaryPath() const;
	void SetGitBinaryPath(const FString& InString);

	const FString& GetRepoRootPath() const;
	void SetRepoRootPath(const FString& InString);

	/** Load settings from ini file */
	void LoadSettings();

	/** Save settings to ini file */
	void SaveSettings() const;

private:
	/** A critical section for settings access */
	mutable FCriticalSection CriticalSection;

	FString ServerUrl;
	FString UserName;
	FString GitBinaryPath;
	FString RepoRootPath;
};
