// ----------------------------------------------------------------------------
// © 2022 Eungsik Yoon <yoon.eungsik@gmail.com>
// ----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SEditableTextBox.h"

class SUeLfsSettings : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SUeLfsSettings) {}

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

	FText GetServerUrlText() const;
	void OnServerUrlTextCommited(const FText& InText, ETextCommit::Type InCommitType) const;

	FText GetUserNameText() const;
	void OnUserNameTextCommited(const FText& InText, ETextCommit::Type InCommitType) const;

	FText GetGitBinaryPathText() const;

	FText GetRepoRootPathText() const;
	void OnRepoRootPathTextCommited(const FText& InText, ETextCommit::Type InCommitType) const;

private:

	mutable FCriticalSection CriticalSection;

	FString ServerUrl;
	FString UserName;
	FString GitBinaryPath;
};
