// ----------------------------------------------------------------------------
// © 2022 Eungsik Yoon <yoon.eungsik@gmail.com>
// ----------------------------------------------------------------------------

#include "UeLfsSettings.h"
#include "Misc/ScopeLock.h"
#include "Misc/ConfigCacheIni.h"
#include "SourceControlHelpers.h"
#include "UeLfsUtils.h"

namespace UeLfsSettingsConstants
{
	/** The section of the ini file we load our settings from */
	static const FString SettingsSection = TEXT("UeLfs.UeLfsSettings");
}

const FString& FUeLfsSettings::GetServerUrl() const
{
	FScopeLock ScopeLock(&CriticalSection);
	return ServerUrl;
}

void FUeLfsSettings::SetServerUrl(const FString& InString)
{
	FScopeLock ScopeLock(&CriticalSection);
	ServerUrl = InString;
}

const FString& FUeLfsSettings::GetUserName() const
{
	FScopeLock ScopeLock(&CriticalSection);
	return UserName;
}

void FUeLfsSettings::SetUserName(const FString& InString)
{
	FScopeLock ScopeLock(&CriticalSection);
	UserName = InString;
}

const FString& FUeLfsSettings::GetGitBinaryPath() const
{
	FScopeLock ScopeLock(&CriticalSection);
	return GitBinaryPath;
}

void FUeLfsSettings::SetGitBinaryPath(const FString& InString)
{
	FScopeLock ScopeLock(&CriticalSection);
	GitBinaryPath = InString;
}

const FString& FUeLfsSettings::GetRepoRootPath() const
{
	FScopeLock ScopeLock(&CriticalSection);
	return RepoRootPath;
}

void FUeLfsSettings::SetRepoRootPath(const FString& InString)
{
	FScopeLock ScopeLock(&CriticalSection);
	RepoRootPath = InString;
}

void FUeLfsSettings::LoadSettings()
{
	FScopeLock ScopeLock(&CriticalSection);
	const FString& IniFile = SourceControlHelpers::GetSettingsIni();
	GConfig->GetString(*UeLfsSettingsConstants::SettingsSection, TEXT("ServerUrl"), ServerUrl, IniFile);
	GConfig->GetString(*UeLfsSettingsConstants::SettingsSection, TEXT("UserName"), UserName, IniFile);
	GConfig->GetString(*UeLfsSettingsConstants::SettingsSection, TEXT("GitBinaryPath"), GitBinaryPath, IniFile);
	GConfig->GetString(*UeLfsSettingsConstants::SettingsSection, TEXT("RepoRootPath"), RepoRootPath, IniFile);
}

void FUeLfsSettings::SaveSettings() const
{
	FScopeLock ScopeLock(&CriticalSection);
	const FString& IniFile = SourceControlHelpers::GetSettingsIni();
	GConfig->SetString(*UeLfsSettingsConstants::SettingsSection, TEXT("ServerUrl"), *ServerUrl, IniFile);
	GConfig->SetString(*UeLfsSettingsConstants::SettingsSection, TEXT("UserName"), *UserName, IniFile);
	GConfig->SetString(*UeLfsSettingsConstants::SettingsSection, TEXT("GitBinaryPath"), *GitBinaryPath, IniFile);
	GConfig->SetString(*UeLfsSettingsConstants::SettingsSection, TEXT("RepoRootPath"), *RepoRootPath, IniFile);
}
