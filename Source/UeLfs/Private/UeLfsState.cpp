// ----------------------------------------------------------------------------
// © 2022 Eungsik Yoon <yoon.eungsik@gmail.com>
// ----------------------------------------------------------------------------

#include "UeLfsState.h"

#define LOCTEXT_NAMESPACE "UeLfs.State"

int32 FUeLfsState::GetHistorySize() const
{
	return History.Num();
}

TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FUeLfsState::GetHistoryItem( int32 HistoryIndex ) const
{
	check(History.IsValidIndex(HistoryIndex));
	return History[HistoryIndex];
}

TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FUeLfsState::FindHistoryRevision( int32 RevisionNumber ) const
{
	return nullptr;
}

TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FUeLfsState::FindHistoryRevision(const FString& InRevision) const
{
	return nullptr;
}

TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FUeLfsState::GetBaseRevForMerge() const
{
	return nullptr;
}

FName FUeLfsState::GetIconName() const
{
	if (LockState == ELockState::Locked)
	{
		return FName("Subversion.CheckedOut");
	}
	else if (LockState == ELockState::LockedOther)
	{
		return FName("Subversion.CheckedOutByOtherUser");
	}
	else if (LockState == ELockState::NotCurrent)
	{
		return FName("Subversion.NotAtHeadRevision");
	}

	return NAME_None;
}

FName FUeLfsState::GetSmallIconName() const
{
	if (LockState == ELockState::Locked)
	{
		return FName("Subversion.CheckedOut_Small");
	}
	else if (LockState == ELockState::LockedOther)
	{
		return FName("Subversion.CheckedOutByOtherUser_Small");
	}
	else if (LockState == ELockState::NotCurrent)
	{
		return FName("Subversion.NotAtHeadRevision_Small");
	}

	return NAME_None;
}

FText FUeLfsState::GetDisplayName() const
{
	if (LockState == ELockState::Locked)
	{
		return LOCTEXT("Locked", "Locked For Editing");
	}
	else if (LockState == ELockState::LockedOther)
	{
		return FText::Format( LOCTEXT("LockedOther", "Locked by "), FText::FromString(LockUser) );
	}
	else if (LockState == ELockState::NotCurrent)
	{
		return LOCTEXT("LockNotCurrent", "Not current.");
	}

	return FText();
}

FText FUeLfsState::GetDisplayTooltip() const
{
	if (LockState == ELockState::Locked)
	{
		return LOCTEXT("Locked_Tooltip", "Locked for editing by current user");
	}
	else if (LockState == ELockState::LockedOther)
	{
		return FText::Format( LOCTEXT("LockedOther_Tooltip", "Locked for editing by: {0}"), FText::FromString(LockUser) );
	}
	else if (LockState == ELockState::NotCurrent)
	{
		return LOCTEXT("LockNotCurrent_Tooltip", "Not current.");
	}

	return FText();
}

const FString& FUeLfsState::GetFilename() const
{
	return LocalFileName;
}

const FDateTime& FUeLfsState::GetTimeStamp() const
{
	return TimeStamp;
}

bool FUeLfsState::CanCheckIn() const
{
	return LockState == ELockState::Locked;
}

bool FUeLfsState::CanCheckout() const
{
	// Newly added files don't care about locking.
	if (WorkingCopyState == EWorkingCopyState::Added)
	{
		return false;
	}

	return LockState == ELockState::NotLocked;
}

bool FUeLfsState::IsCheckedOut() const
{
	return LockState == ELockState::Locked;
}

bool FUeLfsState::IsCheckedOutOther(FString* Who) const
{
	if (Who != NULL)
	{
		*Who = LockUser;
	}

	return LockState == ELockState::LockedOther;
}

bool FUeLfsState::IsCurrent() const
{
	return true;
}

bool FUeLfsState::IsSourceControlled() const
{
	// Newly added files don't care about locking.
	if (WorkingCopyState == EWorkingCopyState::Added)
	{
		return false;
	}

	return WorkingCopyState != EWorkingCopyState::NotControlled;
}

bool FUeLfsState::IsAdded() const
{
	return WorkingCopyState == EWorkingCopyState::Added;
}

bool FUeLfsState::IsDeleted() const
{
	return WorkingCopyState == EWorkingCopyState::Deleted;
}

bool FUeLfsState::IsIgnored() const
{
	return false; // Nothing's ignored.
}

bool FUeLfsState::CanEdit() const
{
	return LockState == ELockState::Locked;
}

bool FUeLfsState::CanDelete() const
{
	return !IsCheckedOutOther() && IsSourceControlled() && IsCurrent();
}

bool FUeLfsState::IsUnknown() const
{
	return false;
}

bool FUeLfsState::IsModified() const
{
	return WorkingCopyState == EWorkingCopyState::Modified;
}

bool FUeLfsState::CanAdd() const
{
	return false;
}

bool FUeLfsState::IsConflicted() const
{
	return false;
}

bool FUeLfsState::CanRevert() const
{
	return CanCheckIn();
}

#undef LOCTEXT_NAMESPACE
