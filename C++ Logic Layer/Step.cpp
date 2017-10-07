/// \file       Step.cpp

#include "Step.hpp"
#include "Logging.hpp"

Step::Step(StepType type){
    Logging::LogEnterFunction(__FUNCTION__, "");
    this->type = type;
}

Step::Step(StepType type, Task* task){
    Logging::LogEnterFunction(__FUNCTION__, "");
    this->type = type;
    this->task = task;
}

Step::Step(StepType type, int param){
    Logging::LogEnterFunction(__FUNCTION__, "");
    this->type = type;
    intParam = param;
}

Step::Step(StepType type, std::string param){
    Logging::LogEnterFunction(__FUNCTION__, "");
    this->type = type;
    stringParam = param;
}

Step::Step(StepType type, int iparam, std::string sparam){
    Logging::LogEnterFunction(__FUNCTION__, "");
    this->type = type;
    intParam = iparam;
    stringParam = sparam;
}

Step::~Step(void){
    Logging::LogEnterFunction(__FUNCTION__, "");

}

StepType Step::GetType(){
    Logging::LogEnterFunction(__FUNCTION__, "");
    return type;
}

bool Step::DoStep(Hal& hal, Database& database, IQueueHandler& queueHandler){
    Logging::LogEnterFunction(__FUNCTION__, "");
    int ret = 0;
    int drawer = 0;

    switch(type){
        case StepType::DRAWER_EXTEND:
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > DoStep > DRAWER_EXTEND");
            hal.openDrawer(intParam);
            return true;
        case StepType::ALL_DRAWERS_RETRACT:
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > DoStep > ALL_DRAWERS_RETRACT");
            while(ret == 0){
                ret = hal.closeDrawer(drawer);
                drawer++;
            }
            return true;
        case StepType::MAGNET:
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > DoStep > MAGNET");
            hal.setMagnet(intParam);
            return true;
            case StepType::CRANE_MOVE:
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > DoStep > CRANE_MOVE");
            hal.moveCrane(intParam);
            return true;
        case StepType::SEND_RETURN_MESSAGE:
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > DoStep > SEND_RETURN_MESSAGE");
            queueHandler.AddTask(*task);
            return false;
        case StepType::STOP_HAL:
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > DoStep > STOP_HAL");
            hal.de_init();
            return true;
		case StepType::START_HAL:
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > DoStep > START_HAL");
			hal.init();
			return true;
    }
    return false;
}
