/// \file       Logic.hpp
/// \brief      Header file for main logic class
///             Logic class handles all incoming messages from the API layer and controls the HAL layer.

#pragma once

#include <queue>

#include "IHandlerCB.h"
#include "Error.h"

#include "Step.hpp"
#include "Task.h"
#include "hal.hpp"
#include "IQueueHandler.h"
#include "Database.hpp"
#include "Logging.hpp"

    #define CRANE_HOME 0
enum class Resultcodes {
	Success = 0,
	CommunicationError = -1,
	UnknownMessage = -2,
	ParameterCountError = -3,
	InvalidParameter = -4,
	ServerBusy = -5,
	ActionNotPerformedDueToState = -6,
	DrawersFull = -7,
	FilterCombinationError = -8
};

/// \brief      Main logic functionality
class Logic : public IHandlerCB
{
public:
    /// \brief      Constructor
    /// \pre        None.
    /// \post       Initialized logic and HAL.
    /// \param[in]  queueHandler Link to queue object of API layer for return messages
    /// \returns    Nothing
    Logic(IQueueHandler* queueHandler);

    /// \brief      Destructor
    /// \pre        Initialized logic.
    /// \post       Cleaned HAL and logic.
    /// \returns    Nothing
    ~Logic(void);

    /// \brief      Main run function of logic, runs the first step in its queue.
    /// \pre        None. (Preferably added tasks using queue callback)
    /// \post       A step has been executed, next step will be run next call to prevent blocking.
    /// \returns    Void
    void Run(void);

    /// \brief      Saves database to disk
    /// \pre        None.
    /// \post       Database.txt file has been created
    /// \returns    Void
    void Save(void);

    /// \brief      Callback inherited from IHandlerCB, adds task to queue for processing when calling Run().
    /// \pre        None.
    /// \post       The task had been converted to steps and added to the stepQueue
    /// \param[in]  task Task to be executed by the logic layer
    /// \returns    Void
    /// \todo       Replace callback with std::function conform coding standard.
    void callback(Task task);

private:
    /// \brief      Reference to cabinet hardware for storing filters
    Hal* hal;
    /// \brief      Pointer to database for storing information
    Database* database;
    /// \brief      Reference to queue handler for return messages
    IQueueHandler* queueHandler;
    /// \brief      Queue of steps that need to be executed and do not need to wait for hal
    std::queue<Step*> queue;
	/// \brief      Filter combination currently placed
	Combination* placedCombination;

    /// \brief      Constants for crane position
    
    const int cranePositions[5] = {140,105,75,35,CRANE_HOME};
    // static const int craneDrawer3 = 35;
    // static const int craneDrawer2 = 70;
    // static const int craneDrawer1 = 105;
    // static const int craneDrawer0 = 135;
    /// \brief      Constants for magnet operation
    static const int magnetOn = 1;
    static const int magnetOff = 0;

    /// \brief      Converts a task into smaller steps and adds these to the stepQueue.
    /// \pre        None.
    /// \post       A number of steps have been added to the stepQueue.
    /// \param[in]  task Task to be converted and added to stepQueue
    /// \returns    Void
    void taskToStep(Task task);


};