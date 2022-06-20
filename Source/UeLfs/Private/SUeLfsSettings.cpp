// ----------------------------------------------------------------------------
// © 2022 Eungsik Yoon <yoon.eungsik@gmail.com>
// ----------------------------------------------------------------------------

#include "SUeLfsSettings.h"
#include "Modules/ModuleManager.h"
#include "ISourceControlModule.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "EditorStyleSet.h"
#include "UeLfsModule.h"

#define LOCTEXT_NAMESPACE "SUeLfsSettings"

void SUeLfsSettings::Construct(const FArguments& InArgs)
{
	FSlateFontInfo Font = FEditorStyle::GetFontStyle(TEXT("SourceControl.LoginWindow.Font"));

	ChildSlot
	[
		SNew( SBorder )
		.BorderImage( FEditorStyle::GetBrush("DetailsView.CategoryBottom") )
		.Padding( FMargin( 0.0f, 3.0f, 0.0f, 0.0f ) )
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(2.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ServerUrlLabel", "Server URL"))
					.ToolTipText(LOCTEXT("ServerUrlLabel_Tooltip", "UeLfs server URL."))
					.Font(Font)
				]
				+SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(2.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("UserNameLabel", "User Name"))
					.ToolTipText(LOCTEXT("UserNameLabel_Tooltip", "UeLfs username"))
					.Font(Font)
				]
				+SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(2.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("GitBinaryPathLabel", "Git Path"))
					.ToolTipText(LOCTEXT("GitBinaryPathLabel_Tooltip", "Git binary path."))
					.Font(Font)
				]
				+SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(2.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("RepoRootPathLabel", "Repository Root Path"))
					.ToolTipText(LOCTEXT("RepoRootPathLabel_Tooltip", "Repository root path."))
					.Font(Font)
				]
			]
			+SHorizontalBox::Slot()
			.FillWidth(2.0f)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(2.0f)
				[
					SNew(SEditableTextBox)
					.Text(this, &SUeLfsSettings::GetServerUrlText)
					.ToolTipText(LOCTEXT("ServerUrlLabel_Tooltip", "Address of UeLfs server."))
					.OnTextCommitted(this, &SUeLfsSettings::OnServerUrlTextCommited)
					.OnTextChanged(this, &SUeLfsSettings::OnServerUrlTextCommited, ETextCommit::Default)
					.Font(Font)
				]
				+SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(2.0f)
				[
					SNew(SEditableTextBox)
					.Text(this, &SUeLfsSettings::GetUserNameText)
					.ToolTipText(LOCTEXT("UserNameLabel_Tooltip", "UeLfs username"))
					.OnTextCommitted(this, &SUeLfsSettings::OnUserNameTextCommited)
					.OnTextChanged(this, &SUeLfsSettings::OnUserNameTextCommited, ETextCommit::Default)
					.Font(Font)
				]
				+SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(2.0f)
				[
					SNew(STextBlock)
					.Text(this, &SUeLfsSettings::GetGitBinaryPathText)
					.ToolTipText(LOCTEXT("GitBinaryPathLabel_Tooltip", "Git binary path."))
					.Font(Font)
				]
				+SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(2.0f)
				[
					SNew(SEditableTextBox)
					.Text(this, &SUeLfsSettings::GetRepoRootPathText)
					.ToolTipText(LOCTEXT("RepoRootPathLabel_Tooltip", "Repository root path."))
					.OnTextCommitted(this, &SUeLfsSettings::OnRepoRootPathTextCommited)
					.OnTextChanged(this, &SUeLfsSettings::OnRepoRootPathTextCommited, ETextCommit::Default)
					.Font(Font)
				]
				//+ SVerticalBox::Slot()
				//.FillHeight(2.0f)
				//.Padding(2.0f)
				//[
				//	SNew(SButton)
				//	.ContentPadding(4)
				//	.VAlign(VAlign_Center)
				//	.HAlign(HAlign_Center)
				//	.OnClicked(this, &SUeLfsSettings::OnReleaseLocks)
				//	.Text(NSLOCTEXT("ReleaseLocksLabel", "ReleaseLocks", "Release locks."))
				//]
			]
		]
	];
}

FText SUeLfsSettings::GetServerUrlText() const
{
	FUeLfsModule& UeLfs = FModuleManager::LoadModuleChecked<FUeLfsModule>("UeLfs");
	return FText::FromString(UeLfs.AccessSettings().GetServerUrl());
}

void SUeLfsSettings::OnServerUrlTextCommited(const FText& InText, ETextCommit::Type InCommitType) const
{
	FUeLfsModule& UeLfs = FModuleManager::LoadModuleChecked<FUeLfsModule>("UeLfs");
	UeLfs.AccessSettings().SetServerUrl(InText.ToString());
	UeLfs.SaveSettings();
}

FText SUeLfsSettings::GetUserNameText() const
{
	FUeLfsModule& UeLfs = FModuleManager::LoadModuleChecked<FUeLfsModule>("UeLfs");
	return FText::FromString(UeLfs.AccessSettings().GetUserName());
}

void SUeLfsSettings::OnUserNameTextCommited(const FText& InText, ETextCommit::Type InCommitType) const
{
	FUeLfsModule& UeLfs = FModuleManager::LoadModuleChecked<FUeLfsModule>("UeLfs");
	UeLfs.AccessSettings().SetUserName(InText.ToString());
	UeLfs.SaveSettings();
}

void SUeLfsSettings::OnRepoRootPathTextCommited(const FText& InText, ETextCommit::Type InCommitType) const
{
	FUeLfsModule& UeLfs = FModuleManager::LoadModuleChecked<FUeLfsModule>("UeLfs");
	UeLfs.AccessSettings().SetRepoRootPath(InText.ToString());
	UeLfs.SaveSettings();
}

FText SUeLfsSettings::GetGitBinaryPathText() const
{
	FUeLfsModule& UeLfs = FModuleManager::LoadModuleChecked<FUeLfsModule>("UeLfs");
	return FText::FromString(UeLfs.AccessSettings().GetGitBinaryPath());
}

FText SUeLfsSettings::GetRepoRootPathText() const
{
	FUeLfsModule& UeLfs = FModuleManager::LoadModuleChecked<FUeLfsModule>("UeLfs");
	return FText::FromString(UeLfs.AccessSettings().GetRepoRootPath());
}

#undef LOCTEXT_NAMESPACE // "SUeLfsSettings"
