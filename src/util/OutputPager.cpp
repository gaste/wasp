/*
 *
 *  Copyright 2013 Mario Alviano, Carmine Dodaro, and Francesco Ricca.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#include "OutputPager.h"

#include <iostream>
#include <sstream>

unsigned int OutputPager::pageSize = 15;

void
OutputPager::paginate(
    const string& output )
{
	stringstream stream;
	string line, userInput;
	bool printOutput = true;
	unsigned int counter = 0;

	stream << output;

	while ( getline( stream, line ) && printOutput )
	{
		if ( counter < pageSize )
		{
			cout << line << endl;
			counter++;
		}
		else
		{
			cout << endl;
			cout << "-- display more? (y/n)";
			getline( cin, userInput );
			cout << endl;

			if ( userInput == "y" )
			{
				cout << line << endl;
				counter = 1;
			}
			else
				printOutput = false;
		}
	}
}

void
OutputPager::setPageSize (
		const int& size )
{
	pageSize = size;
}
