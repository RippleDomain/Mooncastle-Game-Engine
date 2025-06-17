#pragma once

#include "Test.h"
#include "..\Mooncastle\Components\Entity.h"
#include "..\Mooncastle\Components\Transform.h"

#include <iostream>
#include <ctime>

using namespace mooncastle;

class engineTest : public test
{
public:
	bool initialize() override
	{
		srand((u32)time(nullptr));
		return true;
	}

	void run() override
	{
		do 
		{
			for (u32 i = 0; i < 10000; ++i)
			{
				createRandom();
				removeRandom();
				_numEntities = (u32)_entities.size();

			}
			printResults();
		} while (getchar() != 'q');
	}

	void shutDown() override
	{

	}
private:
	void createRandom() 
	{
		u32 count = rand() % 20;
		
		if (_entities.empty()) count = 1000;
		transform::initInfo transformInfo{};
		gameEntity::entityInfo entityInfo{ &transformInfo };

		while (count > 0) 
		{
			++_added;
			gameEntity::entity entity{ gameEntity::create(entityInfo) };
			assert(entity.isValid() && id::isValid(entity.getId()));
			_entities.push_back(entity);
			assert(gameEntity::isAlive(entity.getId()));
			--count;
		}
	}

	void removeRandom() 
	{
		u32 count = rand() % 20;
		if (_entities.size() < 1000) return;

		while (count > 0)
		{
			const u32 index{ (u32)rand() % (u32)_entities.size() };
			const gameEntity::entity entity{ _entities[index] };
			assert(entity.isValid() && id::isValid(entity.getId()));
			if (entity.isValid())
			{
				gameEntity::remove(entity.getId());
				_entities.erase(_entities.begin() + index);
				assert(!gameEntity::isAlive(entity.getId()));
				++_removed;
			}
			--count;
		}
	}

	void printResults() 
	{
		std::cout << "Entities added: " << _added << std::endl;
		std::cout << "Entities removed: " << _removed << std::endl;
	}

	utl::vector<gameEntity::entity> _entities;

	u32 _added{ 0 };
	u32 _removed{ 0 };
	u32 _numEntities{ 0 };
};