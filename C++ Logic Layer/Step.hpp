/// \file       Step.hpp

#pragma once

#include <string>
#include <iostream>

#include "Error.h"
#include "MessageParameter.h"
#include "hal.hpp"
#include "Database.hpp"
#include "Task.h"
#include "IQueueHandler.h"

///List of possible steps
enum class StepType
{
    DRAWER_EXTEND,              ///< Extends specified drawer
    ALL_DRAWERS_RETRACT,        ///< Retracts all extended drawers
    CRANE_MOVE,                 ///< Move crane to specified position
    MAGNET,                     ///< Set magnet state
    SEND_RETURN_MESSAGE,        ///< Sends a return message to the client
    STOP_HAL,                   ///< Sends stop signal to HAL
	START_HAL                   ///< Sends stop signal to HAL
};

/// \brief Small task to execute in a short amount of time
class Step
{
public:
    /// \brief      Constructor without parameter
    /// \pre        None.
    /// \post       Initialized step object with task parameter.
    /// \param[in]  type Defines the type of step
    /// \returns    Nothing
    Step(StepType type);

    /// \brief      Constructor with task parameter
    /// \pre        None.
    /// \post       Initialized step object with task parameter.
    /// \param[in]  type Defines the type of step
    /// \param[in]  task Task parameter to use
    /// \returns    Nothing
    Step(StepType type, Task* task);

    /// \brief      Constructor with int parameter
    /// \pre        None.
    /// \post       Initialized step object with int parameter.
    /// \param[in]  type Defines the type of step
    /// \param[in]  param Integer parameter to use
    /// \returns    Nothing
    Step(StepType type, int param);

    /// \brief      Constructor with string parameter
    /// \pre        None.
    /// \post       Initialized step object with int parameter.
    /// \param[in]  type Defines the type of step
    /// \param[in]  param String parameter to use
    /// \returns    Nothing
    Step(StepType type, std::string param);

    /// \brief      Constructor with string and int parameter
    /// \pre        None.
    /// \post       Initialized step object with int parameter.
    /// \param[in]  type Defines the type of step
    /// \param[in]  iparam Integer parameter to use
    /// \param[in]  sparam String parameter to use
    /// \returns    Nothing
    Step(StepType type, int iparam, std::string sparam);

    /// \brief      Destructor
    /// \pre        None
    /// \post       Clean step object
    /// \returns    Nothing
    ~Step(void);

    /// \brief      Get the step type of this step object
    /// \pre        None.
    /// \post       None.
    /// \returns    Step type
    StepType GetType(void);

    /// \brief      Execute the step
    /// \pre        None.
    /// \post       Step function has been run
	/// \param[in]  hal Reference to hal.
    /// \param[in]  database Reference to database for storing filter information.
    /// \param[in]  queueHandler Reference to queueHandler.
    /// \returns    True if the step used a HAL function that needs waiting for, false if no waiting is needed.
    bool DoStep(Hal& hal, Database& database, IQueueHandler& queueHandler);

private:
    /// \brief      Stores step's type
    StepType type;
    /// \brief      Stores integer parameter
    int intParam;
    /// \brief      Task for return message
    Task* task;
    /// \brief      Stores string parameter
    std::string stringParam;
};