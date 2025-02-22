/*
 *
 *  Copyright 2015 Mario Alviano, Carmine Dodaro, Francesco Ricca, and Philip
 *  Gasteiger.
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

#include "DebugUserInterfaceCLI.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iterator>
#include <utility>

#include "../util/Formatter.h"
#include "../util/OutputPager.h"
#include "../util/RuleNames.h"
#include "../util/VariableNames.h"

map< string, cmd > DebugUserInterfaceCLI::commandMap =
{
    { "show core", { SHOW_CORE, "Show the literals, ground rules and non-ground rules inside the UNSAT core." } },
    { "show history", { SHOW_HISTORY, "Show the history of assertions." } },
    { "ask", { ASK_QUERY, "Ask me a question about the program." } },
    { "save history", { SAVE_HISTORY, "Save the assertion history in a file." } },
    { "load history", { LOAD_HISTORY, "Load the assertion history from a file." } },
    { "assert", { ASSERT_VARIABLE, "Assert the truth value of a variable." } },
    { "undo assert", { UNDO_ASSERTION, "Choose and undo an assertion." } },
    { "fix core", { ANALYZE_DISJOINT_CORES, "Compute all disjoint cores and fix one of them." } },
    { "exit", { EXIT, "Stop the debugging session." } }
};

UserCommand
DebugUserInterfaceCLI::promptCommand()
{
    if ( forceNextCommand )
    {
        forceNextCommand = false;
        return nextCommand;
    }

	string userInput = "";

	do
	{
		promptInput( userInput );

		if ( "help" == userInput )
		{
		    printHelp();
		}
		else if ( commandMap.count( userInput ) )
		{
		    return commandMap[ userInput ].command;
		}
		else
		{
		    cout << "Undefined command: \"" + userInput + "\".  Try \"help\"." << endl;
		}
	} while ( !cin.eof() );

	return UserCommand::EXIT;
}

void
DebugUserInterfaceCLI::printHelp()
{
    string helpText = "Available commands:\n\n";

    for ( const auto& pair : commandMap )
    {
        helpText += pair.first + " -- " + pair.second.helpText + "\n";
    }

    OutputPager::paginate( helpText );
}

void
DebugUserInterfaceCLI::printCore(
    const vector< Literal >& core,
    const vector< Literal >& coreAssertions )
{
    bool repeat = true;
    string userInput = "";

    do {
        cout << "Display literals (l), ground rules (g) or non-ground rules (n)?> ";
        getline( cin, userInput );

        if ( "l" == userInput )
        {
            printCoreLiterals( core, coreAssertions );
            repeat = false;
        }
        else if ( "g" == userInput )
        {
            printCoreGroundRules( core, coreAssertions );
            repeat = false;
        }
        else if ( "n" == userInput )
        {
            printCoreNonGroundRules( core, coreAssertions );
            repeat = false;
        }
    } while ( repeat );
}

void
DebugUserInterfaceCLI::printCoreLiterals(
    const vector< Literal >& core,
    const vector< Literal >& coreAssertions )
{
    OutputPager::paginate(
            "rules = " + Formatter::formatClause( core ) +
            "\nassertions = " + Formatter::formatClause( coreAssertions ) );
}

void
DebugUserInterfaceCLI::printCoreGroundRules(
    const vector< Literal >& core,
    const vector< Literal >& coreAssertions )
{
    string groundCoreRules = "";
    if ( !core.empty() )
    {
        for ( const Literal& coreLiteral : core )
        {
            groundCoreRules += RuleNames::getGroundRule( coreLiteral ) + "\n";
        }
    }

    OutputPager::paginate(
            (!core.empty() ? "rules:\n" + groundCoreRules : "no rules\n") +
            "assertions = " + Formatter::formatClause( coreAssertions ) );
}

void
DebugUserInterfaceCLI::printCoreNonGroundRules(
    const vector< Literal >& core,
    const vector< Literal >& coreAssertions )
{
    string coreUngroundRules = "";
    map< string, vector< string > > ruleSubstitutionMap;

    for( const Literal& coreLiteral : core )
    {
        string rule = RuleNames::getRule( coreLiteral.getVariable() );
        string substitution = RuleNames::getSubstitution( coreLiteral.getVariable() );

        if ( substitution.length() > 0 )
            ruleSubstitutionMap[ rule ].push_back( substitution );
        else
            ruleSubstitutionMap[ rule ];
    }

    for ( const auto& mapEntry : ruleSubstitutionMap )
    {
        coreUngroundRules += mapEntry.first + "\n";

        for ( const string& substitution : mapEntry.second )
        {
            coreUngroundRules += "    " + substitution + "\n";
        }
    }

    OutputPager::paginate(
            (!core.empty() ? "rules:\n" + coreUngroundRules : "no rules\n") +
            "assertions = " + Formatter::formatClause( coreAssertions ) );
}

void
DebugUserInterfaceCLI::printHistory(
    const vector< Literal >& assertionHistory )
{
    string history = "";

    for ( unsigned int i = 0; i < assertionHistory.size(); i ++ )
    {
        history += to_string( i ) + ": "
                + VariableNames::getName( assertionHistory[ i ].getVariable() )
                + " = "
                + (assertionHistory[ i ].isPositive() ? "true" : "false")
                + "\n";
    }

    OutputPager::paginate( history );
}

void
DebugUserInterfaceCLI::queryResponse(
    const vector< Var >& variables )
{
    if ( variables.empty() )
    {
        cout << "No more queries are possible" << endl;
        return;
    }

    TruthValue val = askTruthValue( variables[ 0 ] );

    if ( UNDEFINED == val ) return;

    forceNextCommand = true;
    nextCommand = UserCommand::ASSERT_VARIABLE;
    nextAssertion = Literal( variables[ 0 ], TRUE == val ? POSITIVE : NEGATIVE );
}

TruthValue
DebugUserInterfaceCLI::askTruthValue(
    const Var variable )
{
	string userInput;

	do
	{
		cout << "Should '" << VariableNames::getName( variable )
			 << "' be in the model? (y/n/u): ";

		getline( cin, userInput );

		transform( userInput.begin(), userInput.end(), userInput.begin(), ::tolower );

		if ( userInput == "y" ) return TRUE;
		else if ( userInput == "n" ) return FALSE;
		else if ( userInput == "u" ) return UNDEFINED;
	} while(true);
}

string
DebugUserInterfaceCLI::askHistoryFilename()
{
    string filename;
    cout << "Filename: ";
    getline( cin, filename );
    return filename;
}

vector< Literal >
DebugUserInterfaceCLI::getAssertions()
{
    if ( Literal::null != nextAssertion )
    {
        vector< Literal > assertions;
        assertions.push_back( Literal( nextAssertion ) );

        nextAssertion = Literal::null;

        return assertions;
    }

    string input;
    Var assertionVariable = 0;

    do
    {
        cout << "Variable: ";
        getline( cin, input );

        if ( !VariableNames::getVariable( input, assertionVariable ) )
        {
            assertionVariable = 0;
            cout << "No variable named \"" << input << "\" exists" << endl;
        }
    } while ( 0 == assertionVariable );

    do
    {
        cout << "Truth value (t/f): ";
        getline( cin, input );
        transform( input.begin(), input.end(), input.begin(), ::tolower );
    } while ( "t" != input && "f" != input );

    vector< Literal > assertions;
    assertions.push_back( Literal( assertionVariable, "t" == input ? POSITIVE : NEGATIVE ) );

    return assertions;
}

unsigned int
DebugUserInterfaceCLI::chooseAssertionToUndo(
    const vector< Literal >& assertionHistory )
{
    if ( assertionHistory.empty() )
    {
        cout << "No assertions available." << endl;
        return 1;
    }

    string userInput;
    unsigned int assertion = assertionHistory.size();
    cout << "Choose an assertion to undo:" << endl;
    printHistory( assertionHistory );

    do
    {
        cout << "Assertion (0-" << (assertionHistory.size() - 1) << "): ";
        getline( cin, userInput );

        // check if the input is a number
        auto iterator = userInput.begin();
        while ( iterator != userInput.end() && isdigit( *iterator ) ) ++iterator;

        if ( !userInput.empty() && iterator == userInput.end() )
            assertion = atoi( userInput.c_str() );
    } while ( assertion >= assertionHistory.size() );

    return assertion;
}


void
DebugUserInterfaceCLI::informUnfoundedCase()
{
    cout << "The core is an unfounded set" << endl;
}

void
DebugUserInterfaceCLI::informPossiblySupportingRule(
    const Literal& unfoundedAssertion,
    const string& supportingRule )
{
    cout << "Possibly supporting rule for atom '" << Formatter::formatLiteral( unfoundedAssertion ) << "':" << endl
         << "  " << RuleNames::getGroundRule( supportingRule ) << endl;
}

void
DebugUserInterfaceCLI::informAnalyzedDisjointCores(
    const unsigned int numCores )
{
    if ( numCores == 1 )
    {
        cout << "There is only one core." << endl;
    }
    else
    {
        cout << "Found " << numCores << " cores and fixed one of them." << endl;
    }
}


void
DebugUserInterfaceCLI::greetUser()
{
    cout << "WASP debugging mode" << endl;
}

void
DebugUserInterfaceCLI::informComputingCore()
{
    cout << "Computing the unsatisfiable core" << endl;
}

void
DebugUserInterfaceCLI::informComputingQuery()
{
    cout << "Computing the query" << endl;
}

void
DebugUserInterfaceCLI::informSavedHistory(
    const string& filename )
{
    cout << "Saved history to '" << filename << "'" << endl;
}

void
DebugUserInterfaceCLI::informLoadedHistory(
    const string& filename )
{
    cout << "Loaded history from '" << filename << "'" << endl;
}

void
DebugUserInterfaceCLI::informCouldNotSaveHistory(
    const string& filename )
{
    cout << "Unable to save the history to the file '" << filename << "'" << endl;
}

void
DebugUserInterfaceCLI::informCouldNotLoadHistory(
    const string& filename )
{
    cout << "Unable to load the history from the file '" << filename << "'" << endl;
}

void
DebugUserInterfaceCLI::informAssertionAlreadyPresent(
    const string& variable )
{
    cout << "The variable \"" << variable << "\" is already an assertion" << endl;
}

void
DebugUserInterfaceCLI::informAssertionIsFact(
    const string& variable )
{
    cout << "The variable \"" << variable << "\" is a fact" << endl;
}

void
DebugUserInterfaceCLI::informProgramCoherent(
    const vector< Var >& answerSet )
{

    cout << "The program is coherent with answer set = { ";

    bool first = true;

    for ( const Var& v : answerSet )
    {
        if (!first)
            cout << ", ";
        else
            first = false;

        cout << VariableNames::getName( v );
    }

    cout << "}." << endl
         << "Add" << endl
         << "    :- not atom." << endl
         << "for atoms expected to be in the answer set and" << endl
         << "    :- atom." << endl
         << "for atoms expected not to be in the answer set to the program." << endl;
}
