/// \file       Logic.cpp

#include "Logic.hpp"
#include <sstream>
#include <iostream>
#include <vector>

#define MAX_DRAWER 4

Logic::Logic(IQueueHandler* queueHandler){
    Logging::LogEnterFunction(__FUNCTION__, "");
    this->queueHandler = queueHandler;
    hal = new Hal();
    hal->init();
	database = new Database(MAX_DRAWER);
	database->LoadFromDisk();
	placedCombination = NULL;
}

Logic::~Logic(void){
    Logging::LogEnterFunction(__FUNCTION__, "");
    hal->de_init();
    delete hal;
    hal = NULL;
	delete database;
}

void Logic::Run(void){
    Logging::LogEnterFunction(__FUNCTION__, "");

    hal->run();

    if (hal->getState() == HalStates::ERROR){
		Logging::LogEvent((int)LogLevels::LogError, "Logic > Run > Hal error state");
        return;
    }
	else if(queue.empty() || hal->getState() == HalStates::BUSY) return;

	Logging::LogEvent((int)LogLevels::LogDebug, "Logic > Run > Next step");

    Step* s = queue.front();
	queue.pop();
    bool waitingForHAL = s->DoStep(*hal, *database, *queueHandler);
    if(waitingForHAL) Logging::LogEvent((int)LogLevels::LogDebug, "Logic > Run > Waiting for hal");
    delete s;
    s = NULL;
}

void Logic::Save(void){
	database->SaveToDisk();
}

void Logic::callback(Task task){
    Logging::LogEnterFunction(__FUNCTION__, "");
    taskToStep(task);
}



void Logic::taskToStep(Task task){
    Logging::LogEnterFunction(__FUNCTION__, "");
    Task* t = new Task(
            task.GetMessageID(),
            task.GetBlockID(),
            task.GetPriority(),
            task.GetCommand(),
            TaskTypeEnum::RESPONSEMESSAGE
    );

    switch (task.GetCommand()){
        case TaskCommandEnum::ADDFILTER:
		{
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > Received task > ADDFILTER");
			Filter* f = new Filter;
			f->index = database->GetFilterCount() + 1;
			task.GetParameter(0)->AsString(&(f->id));
			task.GetParameter(1)->AsString(&(f->material));
			task.GetParameter(2)->AsString(&(f->thickness));
			database->AddFilter(f);
			queue.push(new Step(StepType::CRANE_MOVE, CRANE_HOME));
			queue.push(new Step(StepType::ALL_DRAWERS_RETRACT));
			queue.push(new Step(StepType::CRANE_MOVE, cranePositions[0]));
			queue.push(new Step(StepType::MAGNET, magnetOn));
			queue.push(new Step(StepType::CRANE_MOVE, CRANE_HOME));
			//queue.push(new Step(StepType::CRANE_MOVE, (f->index < MAX_DRAWER-1) ? cranePositions[f->index+1] : MAX_DRAWER-1));
			queue.push(new Step(StepType::DRAWER_EXTEND, f->index));
			queue.push(new Step(StepType::CRANE_MOVE, cranePositions[f->index]));
			queue.push(new Step(StepType::MAGNET, magnetOff));
			queue.push(new Step(StepType::CRANE_MOVE, CRANE_HOME));
			queue.push(new Step(StepType::ALL_DRAWERS_RETRACT));
			t->AddParameter(std::to_string((int)Resultcodes::Success));
			queue.push(new Step(StepType::SEND_RETURN_MESSAGE, t));
		}
		break;
        case TaskCommandEnum::REQUESTADDFILTER:
		{
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > Received task > REQUESTADDFILTER");
			std::string id;
			task.GetParameter(0)->AsString(&id);
			int i = database->HasRoom(id);
			if (i < 0) {
				t->AddParameter(std::to_string((int)Resultcodes::DrawersFull));
				queueHandler->AddTask(*t);
			}
			else {
				queue.push(new Step(StepType::CRANE_MOVE, CRANE_HOME));
				queue.push(new Step(StepType::DRAWER_EXTEND, 0));
				t->AddParameter(std::to_string((int)Resultcodes::Success));
				queue.push(new Step(StepType::SEND_RETURN_MESSAGE, t));
			}
			
		}
		break;
        case TaskCommandEnum::CANCELADDFILTER:
		{
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > Received task > CANCELADDFILTER");
			queue.push(new Step(StepType::ALL_DRAWERS_RETRACT));
			t->AddParameter(std::to_string((int)Resultcodes::Success));
			queueHandler->AddTask(*t);
		}
        break;
        case TaskCommandEnum::REMOVEFILTER:
		{
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > Received task > REMOVEFILTER");
			std::string id;
			task.GetParameter(0)->AsString(&id);
			Filter* f = database->GetFilterById(id);
			if (f == NULL) {
				t->AddParameter(std::to_string((int)Resultcodes::InvalidParameter));
				queueHandler->AddTask(*t);
				return;
			}
			database->RemoveFilter(f);
			queue.push(new Step(StepType::CRANE_MOVE, CRANE_HOME));
			queue.push(new Step(StepType::ALL_DRAWERS_RETRACT));
			t->AddParameter(std::to_string((int)Resultcodes::Success));
			queue.push(new Step(StepType::SEND_RETURN_MESSAGE, t));
		}
        break;
		case TaskCommandEnum::REQUESTREMOVEFILTER: 
		{
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > Received task > REQUESTREMOVEFILTER");
			std::string id;
			task.GetParameter(0)->AsString(&id);
			Filter* f = database->GetFilterById(id);
			if (f == NULL) {
				t->AddParameter(std::to_string((int)Resultcodes::InvalidParameter));
				queueHandler->AddTask(*t);
				return;
			}
			queue.push(new Step(StepType::CRANE_MOVE, CRANE_HOME));
			queue.push(new Step(StepType::ALL_DRAWERS_RETRACT));
			queue.push(new Step(StepType::DRAWER_EXTEND, f->index));
			queue.push(new Step(StepType::CRANE_MOVE, cranePositions[(f->index)]));
			queue.push(new Step(StepType::MAGNET, magnetOn));
			//queue.push(new Step(StepType::CRANE_MOVE, CRANE_HOME));
			
			queue.push(new Step(StepType::ALL_DRAWERS_RETRACT));
			queue.push(new Step(StepType::CRANE_MOVE, cranePositions[(0)]));
			queue.push(new Step(StepType::MAGNET, magnetOff));
			queue.push(new Step(StepType::CRANE_MOVE, CRANE_HOME));
			queue.push(new Step(StepType::DRAWER_EXTEND, 0));
			t->AddParameter(std::to_string((int)Resultcodes::Success));
			queue.push(new Step(StepType::SEND_RETURN_MESSAGE, t));
		}
        break;
        case TaskCommandEnum::CANCELREMOVEFILTER:
		{
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > Received task > CANCELREMOVEFILTER");
			//Currently doesnt place filter back in drawer, needs knowledge of filter
			queue.push(new Step(StepType::CRANE_MOVE, CRANE_HOME));
			queue.push(new Step(StepType::ALL_DRAWERS_RETRACT));
			t->AddParameter(std::to_string((int)Resultcodes::Success));
			queueHandler->AddTask(*t);
		}
        break;
        case TaskCommandEnum::GETFILTERS:
        {
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > Received task > GETFILTERS");
			std::vector<Filter*> filters = database->GetFilters();
			t->AddParameter(std::to_string((int)Resultcodes::Success));
			for (int i = 0; i < (int)filters.size(); i++) {
				t->AddParameter(filters.at(i)->id);
				t->AddParameter(filters.at(i)->material);
				t->AddParameter(filters.at(i)->thickness);
			}
			queueHandler->AddTask(*t);
        }
        break;
        case TaskCommandEnum::ADDFILTERCOMBINATION:
		{
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > Received task > ADDFILTERCOMBINATION");
			Combination* c = new Combination;
			task.GetParameter(0)->AsString(&(c->id));
			task.GetParameter(1)->AsString(&(c->name));
			int nr = 0;
			task.GetParameter(2)->AsInt(&nr);
			for (int i = 0; i < nr; i++) {
				std::string id = "";
				task.GetParameter(i + 3)->AsString(&id);
				Filter* f = database->GetFilterById(id);
				if (f == NULL) {
					t->AddParameter(std::to_string((int)Resultcodes::FilterCombinationError));
					queueHandler->AddTask(*t);
					return;
				}
				c->filters.push_back(f);
			}
			int ret = database->AddFilterCombination(c);
			if (ret < 0) t->AddParameter(std::to_string((int)Resultcodes::InvalidParameter));
			else t->AddParameter(std::to_string((int)Resultcodes::Success));
			queueHandler->AddTask(*t);

		}
        break;
        case TaskCommandEnum::REMOVEFILTERCOMBINATION:
		{
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > Received task > REMOVEFILTERCOMBINATION");
			std::string id = "";
			task.GetParameter(0)->AsString(&id);
			int ret = database->RemoveFilterCombination(id);
			if (ret < 0) t->AddParameter(std::to_string((int)Resultcodes::InvalidParameter));
			else t->AddParameter(std::to_string((int)Resultcodes::Success));
			queueHandler->AddTask(*t);
		}
        break;
        case TaskCommandEnum::GETFILTERCOMBINATIONS:
		{
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > Received task > GETFILTERCOMBINATIONS");
			t->AddParameter(std::to_string((int)Resultcodes::Success));
			std::vector<Combination*> combinations = database->GetFilterCombinations();
			for (int i = 0; i < (int)combinations.size(); i++) {
				Combination* c = combinations.at(i);
				t->AddParameter(c->id);
				t->AddParameter(c->name);
				t->AddParameter(c->placed ? "true" : "false");
				t->AddParameter(std::to_string(c->filters.size()));
				for (int j = 0; j < (int)c->filters.size(); j++) {
					t->AddParameter(c->filters.at(j)->id);
				}
			}
			queueHandler->AddTask(*t);
		}
        break;
        case TaskCommandEnum::PLACECOMBINATION:
		{
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > Received task > PLACECOMBINATION");
			std::string id = "";
			task.GetParameter(0)->AsString(&id);
			placedCombination = database->GetPlacedCombination();
			if(placedCombination != NULL){
				t->AddParameter(std::to_string((int)Resultcodes::FilterCombinationError));
				queueHandler->AddTask(*t);
				Logging::LogEvent((int)LogLevels::LogDebug, "Logic > PLACECOMBINATION > Combination already placed");
				return;
			}
			placedCombination = database->GetFilterCombination(id);
			if (placedCombination == NULL) {
				t->AddParameter(std::to_string((int)Resultcodes::FilterCombinationError));
				queueHandler->AddTask(*t);
				Logging::LogEvent((int)LogLevels::LogDebug, "Logic > PLACECOMBINATION > Combination not found");
				return;
			}
			queue.push(new Step(StepType::ALL_DRAWERS_RETRACT));
			queue.push(new Step(StepType::CRANE_MOVE, CRANE_HOME));
			for (int i = 0; i < (int)placedCombination->filters.size(); i++) {
				Filter* f = placedCombination->filters.at(i);
				queue.push(new Step(StepType::DRAWER_EXTEND, f->index));
				queue.push(new Step(StepType::CRANE_MOVE, cranePositions[(f->index)]));
				queue.push(new Step(StepType::MAGNET, magnetOn));
				queue.push(new Step(StepType::CRANE_MOVE, CRANE_HOME));
				queue.push(new Step(StepType::ALL_DRAWERS_RETRACT));
				queue.push(new Step(StepType::CRANE_MOVE, cranePositions[(0)] - (i * 10)));
				queue.push(new Step(StepType::MAGNET, magnetOff));
				queue.push(new Step(StepType::CRANE_MOVE, CRANE_HOME));
			}
			placedCombination->placed = true;
			queue.push(new Step(StepType::DRAWER_EXTEND, 0));
			Task* cb = new Task(
				task.GetMessageID(),
				task.GetBlockID(),
				task.GetPriority(),
				TaskCommandEnum::PLACEFILTERCOMBINATIONCALLBACK,
				TaskTypeEnum::RESPONSEMESSAGE
			);
			cb->AddParameter(std::to_string((int)Resultcodes::Success));
			queue.push(new Step(StepType::SEND_RETURN_MESSAGE, cb));
			t->AddParameter(std::to_string((int)Resultcodes::Success));
			queueHandler->AddTask(*t);
		}
		break;
        case TaskCommandEnum::REMOVECOMBINATION:
		{
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > Received task > REMOVECOMBINATION");
			placedCombination = database->GetPlacedCombination();
			if (placedCombination == NULL) {
				t->AddParameter(std::to_string((int)Resultcodes::FilterCombinationError));
				queueHandler->AddTask(*t);
				Logging::LogEvent((int)LogLevels::LogDebug, "Logic > REMOVECOMBINATION > No combination placed");
				return;
			}
			queue.push(new Step(StepType::ALL_DRAWERS_RETRACT));
			queue.push(new Step(StepType::CRANE_MOVE, CRANE_HOME));
			for (int i = placedCombination->filters.size() - 1; i >= 0; i--) {
				Filter* f = placedCombination->filters.at(i);
				queue.push(new Step(StepType::CRANE_MOVE, cranePositions[(0)] - (i * 10)));
				queue.push(new Step(StepType::MAGNET, magnetOn));
				queue.push(new Step(StepType::CRANE_MOVE, CRANE_HOME));
				queue.push(new Step(StepType::DRAWER_EXTEND, f->index));
				queue.push(new Step(StepType::CRANE_MOVE, cranePositions[(f->index)]));
				queue.push(new Step(StepType::MAGNET, magnetOff));
				queue.push(new Step(StepType::CRANE_MOVE, CRANE_HOME));
				queue.push(new Step(StepType::ALL_DRAWERS_RETRACT));
			}
			placedCombination->placed = false;
			Task* cb = new Task(
				task.GetMessageID(),
				task.GetBlockID(),
				task.GetPriority(),
				TaskCommandEnum::REMOVEFILTERCOMBINATIONCALLBACK,
				TaskTypeEnum::RESPONSEMESSAGE
				);
			cb->AddParameter(std::to_string((int)Resultcodes::Success));
			queue.push(new Step(StepType::SEND_RETURN_MESSAGE, cb));
			placedCombination = NULL;
			t->AddParameter(std::to_string((int)Resultcodes::Success));
			queueHandler->AddTask(*t);
		}
        break;
        case TaskCommandEnum::GETSYSTEMSTATUS:
		{
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > Received task > GETSYSTEMSTATUS");
            t->AddParameter(std::to_string((int)Resultcodes::Success));
            t->AddParameter("Nominal");
            t->AddParameter("1.0");
            int emptyDrawerCount = this->database->GetMaxFilterCount() - this->database->GetFilterCount();
            t->AddParameter(std::to_string(emptyDrawerCount));
            queueHandler->AddTask(*t);
		}
        break;
        case TaskCommandEnum::GETSYSTEMLOG:
		{
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > Received task > GETSYSTEMLOG");
			std::stringstream ss;
			std::vector<std::string> events =  Logging::GetEvents();
			for(unsigned int i = 0; i < events.size(); i++){
				ss << events[i] << std::endl;
			}	
			t->AddParameter(std::to_string((int)Resultcodes::Success));
            if (ss.str() == "") 
            {
                ss << "Empty" << std::endl;
            }
            t->AddParameter(ss.str());
			queueHandler->AddTask(*t);
		}
        break;
        case TaskCommandEnum::STOP:
		{
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > Received task > STOP");
			t->AddParameter(std::to_string((int)Resultcodes::Success));
			queueHandler->AddTask(*t);
			queue.push(new Step(StepType::STOP_HAL));
		}
        break;
        case TaskCommandEnum::RESET:
		{
			Logging::LogEvent((int)LogLevels::LogDebug, "Logic > Received task > RESET");
			t->AddParameter(std::to_string((int)Resultcodes::Success));
			queueHandler->AddTask(*t);
			queue.push(new Step(StepType::START_HAL));
		}
        break;
        case TaskCommandEnum::PLACEFILTERCOMBINATIONCALLBACK:
		{
			Logging::LogEvent((int)LogLevels::LogWarning, "Logic > Received task > PLACEFILTERCALLBACK");
			t->AddParameter(std::to_string((int)Resultcodes::UnknownMessage));
			queueHandler->AddTask(*t);
			//Shouldn't get this command
		}
        break;
        case TaskCommandEnum::REMOVEFILTERCOMBINATIONCALLBACK:
		{
			Logging::LogEvent((int)LogLevels::LogWarning, "Logic > Received task > REMOVEFILTERCALLBACK");
			t->AddParameter(std::to_string((int)Resultcodes::UnknownMessage));
			queueHandler->AddTask(*t);
			//Shouldn't get this command
		}
		break;
    }
}
