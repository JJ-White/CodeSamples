/// \file       Database.hpp
/// \brief      Header file for main database class
///             Database class stores information for the logic layer

#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Error.h"
#include "Logging.hpp"

/// \brief      Struct for defining filters
typedef struct {
	int index = -1;
	std::string id = "";
	std::string material = "";
	std::string thickness = "";
}Filter;

/// \brief      Struct for combining filters into combinations
typedef struct{
	std::string id = "";
	std::string name = "";
    bool placed = false;
	std::vector<Filter*> filters;
}Combination;

/// \brief      Database class
class Database
{
public:
    /// \brief      Constructor
    /// \pre        None
    /// \post       Initialized database
	/// \param[in]	maxFilterCount Number of filters that can be physically stored
    /// \returns    Nothing
    Database(int maxFilterCount);

    /// \brief      Destructor
    /// \pre        Initialized database.
    /// \post       Cleaned database
    /// \returns    Nothing
    ~Database(void);

    /// \brief      Save database to file on disk
	/// \pre        None
	/// \post       File created on disk
	/// \returns    -1 on error, 0 on success
    int SaveToDisk();

    /// \brief      Save database to file on disk
	/// \pre        None
	/// \post       File created on disk
	/// \returns    -1 on error, 0 on success
    int LoadFromDisk();

	/// \brief      Check if there is room for a new filter and check if ID already exists
	/// \pre        None
	/// \post       Nothing
	/// \returns    -1 if no room, next free index otherwise
	int HasRoom(std::string id);

    /// \brief      Get all filters
    /// \pre        None
    /// \post       Nothing
    /// \returns    Vector containing filters (pointers)
	std::vector<Filter*> GetFilters(void);

	/// \brief      Get number of filters in database
	/// \pre        None
	/// \post       Nothing
	/// \returns    Number of filters
	int GetFilterCount(void);

    /// \brief      Get total drawers in hardware
	/// \pre        None
	/// \post       Nothing
	/// \returns    Number of drawers
    int GetMaxFilterCount(void);

    /// \brief      Remove filter
    /// \pre        None
    /// \post       Filter removed from list and all combinations using the filter removed
    /// \param[in]  filter Filter to remove
    /// \returns    0 on success, -1 on error
    int RemoveFilter(Filter* filter);

    /// \brief      Add filter
    /// \pre        None
    /// \post       Added filter information to database
    /// \param[in]  filter Pointer to filter on HEAP
    /// \returns    0 on success, -1 on error
    int AddFilter(Filter* filter);

    /// \brief      Get filter combinations
    /// \pre        None
    /// \post       Nothing
    /// \returns	Vector with filter combinations (pointers)
	std::vector<Combination*> GetFilterCombinations(void);

	/// \brief      Get filter combination by id
	/// \pre        None
	/// \post       Nothing
	/// \returns	Filter combination pointer
	Combination* GetFilterCombination(std::string id);

    /// \brief      Add filter combination
    /// \pre        None
    /// \post       Added filter combination to database
    /// \param[in]  combination Pointer to filter combination on HEAP
    /// \returns    0 on success, -1 on error
    int AddFilterCombination(Combination* combination);

    /// \brief      Remove filter combination
    /// \pre        None
    /// \post       Removed filter combination to database
    /// \param[in]  id ID to remove
    /// \returns    0 on success, -1 on error
    int RemoveFilterCombination(std::string id);

	/// \brief      Get filter by unique ID
	/// \pre        None
	/// \post       Nothing
	/// \param[in]  id ID of filter to return
	/// \returns    Filter pointer on success, NULL otherwise
	Filter* GetFilterById(std::string id);

    /// \brief      Get placed filter combination
	/// \pre        None
	/// \post       Nothing
	/// \returns    Filter combination that is placed, or NULL if no combination is placed
	Combination* GetPlacedCombination();

private:
	/// \brief      Number of filters that can be physically stored
	int maxFilterCount;
    /// \brief      List for storing drawer information
    std::vector<Filter*> filters;
    /// \brief      List for storing filter combinations
    std::vector<Combination*> combinations;
	/// \brief      Remove all filter combinations containing this filter
	void RemoveFilterCombinationContaining(Filter* filter);
	/// \brief      Check if another filter is already on this id
	bool IdExists(std::string id);
	/// \brief      Check if another combination is already on this id
	bool CombinationIdExists(std::string id);
    /// \brief      Filename and path for persistent database
	const std::string filename = "database.txt";
    /// \brief      Char for splitting strings
	const char splitChar = ',';
    /// \brief      Char for ending lines
	const char endlChar = ';';
};