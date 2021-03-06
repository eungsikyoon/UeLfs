// ----------------------------------------------------------------------------
// © 2022 Eungsik Yoon <yoon.eungsik@gmail.com>
// ----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"

class IUeLfsWorker
{
public:
	/**
	 * Name describing the work that this worker does. Used for factory method hookup.
	 */
	virtual FName GetName() const = 0;

	/**
	 * Function that actually does the work. Can be executed on another thread.
	 */
	virtual bool Execute( class FUeLfsCommand& InCommand ) = 0;

	/**
	 * Updates the state of any items after completion (if necessary). This is always executed on the main thread.
	 * @returns true if states were updated
	 */
	virtual bool UpdateStates() const = 0;
};

typedef TSharedRef<IUeLfsWorker, ESPMode::ThreadSafe> FUeLfsWorkerRef;
