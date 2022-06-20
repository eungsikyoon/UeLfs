// ----------------------------------------------------------------------------
// © 2022 Eungsik Yoon <yoon.eungsik@gmail.com>
// ----------------------------------------------------------------------------

#include "UeLfsRevision.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "UeLfsModule.h"
#include "UeLfsUtils.h"
#include "Logging/MessageLog.h"

#define LOCTEXT_NAMESPACE "UeLfs"

bool FUeLfsRevision::Get( FString& InOutFilename ) const
{
	//SubversionSourceControlUtils::CheckFilename(Filename);

	FUeLfsModule& UeLfs = FModuleManager::LoadModuleChecked<FUeLfsModule>( "UeLfs" );
	FUeLfsProvider& Provider = UeLfs.GetProvider();

	TArray<FString> Results;
	TArray<FString> Parameters;
	TArray<FString> ErrorMessages;

	// Make temp filename to export to
	FString RevString = (RevisionNumber < 0) ? TEXT("HEAD") : FString::Printf(TEXT("%d"), RevisionNumber);
	FString AbsoluteFileName;
	if(InOutFilename.Len() > 0)
	{
		AbsoluteFileName = InOutFilename;
	}
	else
	{
		// create the diff dir if we don't already have it (SVN wont)
		IFileManager::Get().MakeDirectory(*FPaths::DiffDir(), true);

		static int32 TempFileCount = 0;
		FString TempFileName = FString::Printf(TEXT("%sTemp-%d-Rev-%s-%s"), *FPaths::DiffDir(), TempFileCount++, *RevString, *FPaths::GetCleanFilename(Filename));
		AbsoluteFileName = FPaths::ConvertRelativePathToFull(TempFileName);
	}

	Parameters.Add(FString(TEXT("--revision ")) + RevString);
	Parameters.Add(TEXT("--force"));

	TArray<FString> Files;
	Files.Add(Filename);
	Files.Add(AbsoluteFileName);

	//if(UeLfsUtils::RunCommand(TEXT("export"), Files, Parameters, Results, ErrorMessages, Provider.GetUserName()))
	//{
	//	InOutFilename = AbsoluteFileName;
	//	return true;
	//}
	//else
	//{
	//	for(auto Iter(ErrorMessages.CreateConstIterator()); Iter; Iter++)
	//	{
	//		FMessageLog("SourceControl").Error(FText::FromString(*Iter));
	//	}
	//}

	//return false;

	InOutFilename = AbsoluteFileName;
	return true;
}

static bool IsWhiteSpace(TCHAR InChar)
{
	return InChar == TCHAR(' ') || InChar == TCHAR('\t');
}

static FString NextToken( const FString& InString, int32& InIndex, bool bIncludeWhiteSpace )
{
	FString Result;

	// find first non-whitespace char
	while(!bIncludeWhiteSpace && IsWhiteSpace(InString[InIndex]) && InIndex < InString.Len())
	{
		InIndex++;
	}

	// copy non-whitespace chars
	while(((!IsWhiteSpace(InString[InIndex]) || bIncludeWhiteSpace) && InIndex < InString.Len()))
	{
		Result += InString[InIndex];
		InIndex++;
	}

	return Result;
}

static void ParseBlameResults( const TArray<FString>& InResults, TArray<FAnnotationLine>& OutLines )
{
	// each line is revision number <whitespace> username <whitespace> change
	for(int32 ResultIndex = 0; ResultIndex < InResults.Num(); ResultIndex++)
	{
		const FString& Result = InResults[ResultIndex];

		int32 Index = 0;
		FString RevisionString = NextToken(Result, Index, false);
		FString UserString = NextToken(Result, Index, false);

		// start at Index + 1 here so we don't include an extra space form the SVN output
		FString TextString = NextToken(Result, ++Index, true);

		OutLines.Add(FAnnotationLine(FCString::Atoi(*RevisionString), UserString, TextString));
	}
}

bool FUeLfsRevision::GetAnnotated( TArray<FAnnotationLine>& OutLines ) const
{
	return false;
}

bool FUeLfsRevision::GetAnnotated( FString& InOutFilename ) const
{

	return false;
}

const FString& FUeLfsRevision::GetFilename() const
{
	return Filename;
}

int32 FUeLfsRevision::GetRevisionNumber() const
{
	return RevisionNumber;
}

const FString& FUeLfsRevision::GetRevision() const
{
	return Revision;
}

const FString& FUeLfsRevision::GetDescription() const
{
	return Description;
}

const FString& FUeLfsRevision::GetUserName() const
{
	return UserName;
}

const FString& FUeLfsRevision::GetClientSpec() const
{
	static FString EmptyString(TEXT(""));
	return EmptyString;
}

const FString& FUeLfsRevision::GetAction() const
{
	return Action;
}

TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe> FUeLfsRevision::GetBranchSource() const
{
	return BranchSource;
}

const FDateTime& FUeLfsRevision::GetDate() const
{
	return Date;
}

int32 FUeLfsRevision::GetCheckInIdentifier() const
{
	// in SVN, revisions apply to the whole repository so (in Perforce terms) the revision *is* the changelist
	return RevisionNumber;
}

int32 FUeLfsRevision::GetFileSize() const
{
	return 0;
}

#undef LOCTEXT_NAMESPACE
