/// \file      Database.cpp

#include "Database.hpp"

Database::Database(int maxFilterCount) {
	this->maxFilterCount = maxFilterCount;
}

Database::~Database(void) {
	for (int i = 0; i < (int)filters.size(); i++) {
		delete filters.at(i);
	}
	for (int i = 0; i < (int)combinations.size(); i++) {
		delete combinations.at(i);
	}
}

int Database::SaveToDisk(){
	std::ofstream file;
	file.open(filename, std::ofstream::trunc);
	if(!file.is_open()){
		Logging::LogEvent((int)LogLevels::LogWarning, "Database > SaveToDisk > Unable to open file");
		return -1;
	}

	file << "Filterunit Database File" << std::endl;
	file << "" << std::endl;

	file << "Filters:" << std::endl;
	for(int i = 0; i < (int)filters.size(); i++){
		Filter* f = filters.at(i);
		file << std::to_string(f->index) << ','
		<< f->id << ','
		<< f->material << ','
		<< f->thickness << ';'
		<< std::endl;
	}
	file << "End of filters" << std::endl;
	file << "" << std::endl;
	file << "Combinations:" << std::endl;
	for(int i = 0; i < (int)combinations.size(); i++){
		Combination* c = combinations.at(i);
		file << c->id << ',' <<  c->name << ',' << (c->placed ? '1' : '0')  << ',' << c->filters.size();
		for(int j = 0; j < (int)c->filters.size(); j++){
			file << ',' << c->filters.at(j)->id;
		}
		file << ';' << std::endl;
	}

	file << "End of combinations" << std::endl;
	file << "" << std::endl;
	file << "End of file" << std::endl;
	file.close();
	return 0;
}

int Database::LoadFromDisk(){
	std::ifstream file;
	file.open(filename);
	if(!file.is_open()){
		Logging::LogEvent((int)LogLevels::LogWarning, "Database > LoadFromDisk > Unable to open file");
		return -1;
	}
	bool parsingFilters = false;
	bool parsingCombinations = false;
	std::string line = "";
	while(true){
		std::getline(file, line);

		if(file.eof()) break;
		else if (!file.good()){
			Logging::LogEvent((int)LogLevels::LogWarning, "Database > LoadFromDisk > Read error");
			file.close();
			return -1;
		}

		if(line == "End of filters") parsingFilters = false;
		else if(line == "End of combinations") parsingCombinations = false;
		else if(parsingFilters){
			Filter* f = new Filter();
			std::stringstream ss(line);
			std::string index;
			std::getline(ss, index, splitChar);
			f->index = std::stoi(index);
			std::getline(ss, f->id, splitChar);
			std::getline(ss, f->material, splitChar);
			std::getline(ss, f->thickness, endlChar);

			AddFilter(f);
		}
		else if(parsingCombinations){
			Combination* c = new Combination();
			std::stringstream ss(line);
			std::getline(ss, c->id, splitChar);
			std::getline(ss, c->name, splitChar);
			std::string placed;
			std::getline(ss, placed, splitChar);
			if(placed == "1") c->placed = true;
			else c->placed = false;
			std::string index;
			std::getline(ss, index, splitChar);
			int n = std::stoi(index);
			for(int i = 0; i < n - 1; i++){
				std::string id;
				std::getline(ss, id, splitChar);
				std::cout << id << std::endl;
				Filter* f = GetFilterById(id);
				c->filters.push_back(f);
			}
			std::string id;
			std::getline(ss, id, endlChar);
			Filter* f = GetFilterById(id);
			c->filters.push_back(f);

			AddFilterCombination(c);
		}
		else if(line == "Filters:") parsingFilters = true;
		else if(line == "Combinations:") parsingCombinations = true;
		

	}
	file.close();
	return 0;
}

Combination* Database::GetPlacedCombination(){
	for(int i = 0; i < (int)combinations.size(); i++){
		if(combinations.at(i)->placed){
			return combinations.at(i);
		}
	}
	return NULL;
}

int Database::HasRoom(std::string id){
	if ((int)filters.size() >= maxFilterCount || IdExists(id)) return -1;
	return filters.size() + 1;
}

std::vector<Filter*> Database::GetFilters(void) {
	return filters;
}

int Database::GetFilterCount(void) {
	return (int)filters.size();
}

int Database::GetMaxFilterCount(void) {
    return this->maxFilterCount;
}

int Database::RemoveFilter(Filter* filter) {
	if (filter == NULL) {
		Logging::LogEvent((int)LogLevels::LogWarning, "Database > RemoveFilter > NULL pointer argument");
		return -1;
	}
	RemoveFilterCombinationContaining(filter);
	for (int i = 0; i < (int)filters.size(); i++) {
		if (filters.at(i) == filter) {
			filters.erase(filters.begin() + i);
			delete filter;
			filter = NULL;
			return 0;
		}
	}
	Logging::LogEvent((int)LogLevels::LogWarning, "Database > RemoveFilter > Filter not found");
	return -1;
}

int Database::AddFilter(Filter* filter) {
	if (filter == NULL) {
		Logging::LogEvent((int)LogLevels::LogWarning, "Database > AddFilter > NULL argument");
		return -1;
	}
	if ((int)filters.size() >= maxFilterCount) {
		Logging::LogEvent((int)LogLevels::LogDebug, "Database > AddFilter > No room for new filter");
		return -1;
	}
	if (IdExists(filter->id)) {
		Logging::LogEvent((int)LogLevels::LogDebug, "Database > AddFilter > Filter with ID already exists");
		return -1;
	}
	filter->index = filters.size() + 1;
	filters.push_back(filter);
	return 0;
}

std::vector<Combination*> Database::GetFilterCombinations(void) {
	return combinations;
}

Combination* Database::GetFilterCombination(std::string id) {
	for (int i = 0; i < (int)combinations.size(); i++) {
		Combination* c = combinations.at(i);
		if (c->id == id) {
			return c;
		}
	}
	Logging::LogEvent((int)LogLevels::LogWarning, "Database > GetFilterCombination > Combination not found");
	return NULL;
}

int Database::AddFilterCombination(Combination* combination) {
	if (combination == NULL) {
		Logging::LogEvent((int)LogLevels::LogWarning, "Database > AddCombination > NULL argument");
		return -1;
	}
	if (CombinationIdExists(combination->id)) {
		Logging::LogEvent((int)LogLevels::LogWarning, "Database > AddCombination > Combination ID not unique");
		return -1;
	}
	combinations.push_back(combination);
	return 0;
}

int Database::RemoveFilterCombination(std::string id) {
	for (int i = 0; i < (int)combinations.size(); i++) {
		Combination* c = combinations.at(i);
		if (c->id == id) {
			combinations.erase(combinations.begin() + i);
			delete c;
			return 0;
		}
	}
	Logging::LogEvent((int)LogLevels::LogWarning, "Database > RemoveFilterCombination > Combination not found");
	return -1;
}

Filter* Database::GetFilterById(std::string id) {
	for (int i = 0; i < (int)filters.size(); i++) {
		if (filters.at(i)->id == id) return filters.at(i);
	}
	Logging::LogEvent((int)LogLevels::LogWarning, "Database > GetFilterByID > Filter not found");
	return NULL;
}

void Database::RemoveFilterCombinationContaining(Filter* filter) {
	std::vector<std::string> toBeRemoved;
	for (int i = 0; i < (int)combinations.size(); i++) {
		bool remove = false;
		Combination* c = combinations.at(i);
		for (int j = 0; j < (int)c->filters.size(); j++) {
			if (c->filters.at(j) == filter) remove = true;
		}
		if (remove) toBeRemoved.push_back(c->id);
	}
	for (int i = 0; i < (int)toBeRemoved.size(); i++) {
		RemoveFilterCombination(toBeRemoved.at(i));
	}
}

bool Database::IdExists(std::string id) {
	for (int i = 0; i < (int)filters.size(); i++) {
		if (filters.at(i)->id == id) return true;
	}
	return false;
}

bool Database::CombinationIdExists(std::string id) {
	for (int i = 0; i < (int)combinations.size(); i++) {
		if (combinations.at(i)->id == id) return true;
	}
	return false;
}