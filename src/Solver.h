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

#ifndef SOLVER_H
#define SOLVER_H

#include <cassert>
#include <vector>
using namespace std;

#include "Clause.h"
#include "Variables.h"
#include "Literal.h"
#include "util/Options.h"
#include "util/Trace.h"
#include "stl/UnorderedSet.h"
#include "Learning.h"
#include "outputBuilders/OutputBuilder.h"
#include "util/Assert.h"
#include "Satelite.h"
#include "Restart.h"
#include "MinisatHeuristic.h"
#include "util/Statistics.h"
#include "PostPropagator.h"
#include "DependencyGraph.h"
#include "Aggregate.h"
#include "util/Constants.h"
#include "WatchedList.h"
#include "stl/BoundedQueue.h"
#include "Component.h"
class HCComponent;
class WeakInterface;

class DataStructures
{
    public:
        WatchedList< Clause* > variableWatchedLists;
        Vector< Clause* > variableAllOccurrences;
        Vector< PostPropagator* > variablePostPropagators;
        Vector< pair< Propagator*, int > > variablePropagators;
        Vector< Literal > variableBinaryClauses;

        DataStructures() : isOptimizationLiteral_( false ) {}

        bool isOptLit() const { return isOptimizationLiteral_; }
        void setOptLit( bool value ) { isOptimizationLiteral_ = value; }
    
    private:
        bool isOptimizationLiteral_;
};

struct OptimizationLiteralData
{
    Literal lit;
    uint64_t weight;
    unsigned int level : 30;
    unsigned int removed : 1;
    unsigned int aux : 1;

    void remove() { removed = 1; }
    bool isRemoved() { return removed; }    
    bool isAux() { return aux; }
};

class Solver
{
    public:
        inline Solver();
        ~Solver();
        
        inline void greetings(){ outputBuilder->greetings(); }
        inline void onFinish() { outputBuilder->onFinish(); }
        inline void onKill();
        
        inline unsigned int solve();
        inline unsigned int solve( vector< Literal >& assumptions );
        
        inline void propagate( Var v );
        inline void propagateWithPropagators( Var variable );                
        void propagateAtLevelZero( Var variable );
        void propagateAtLevelZeroSatelite( Var variable );
        void unitPropagation( Var v );
        void shortPropagation( Var v );
        void propagation( Var v );
        void postPropagation( Var v );        

        inline bool preprocessing();
        inline void attachWatches();
        inline void clearComponents();
        inline void clearVariableOccurrences();
        
//        inline void addVariable( const string& name );
        inline void addVariable();
        inline void addVariableRuntime();
        
        inline bool cleanAndAddClause( Clause* clause );
        inline bool addClause( Literal literal );
        inline bool addClause( Clause* clause );
        inline bool addClauseRuntime( Literal literal );
        inline void addBinaryClauseRuntime( Clause* clause );
        inline bool addClauseRuntime( Clause* clause );
        inline bool addClause( Literal lit1, Literal lit2 );        
        
        inline bool addClauseFromModel( Clause* clause );
        void addLearnedClause( Clause* learnedClause, bool optimizeBinary );
        bool addClauseFromModelAndRestart();
        
        inline Literal getLiteral( int lit );
//        inline Var getVariable( unsigned int var );
        
        inline Var getNextVariableToPropagate();
        inline bool hasNextVariableToPropagate() const;        
        
        inline Literal getOppositeLiteralFromLastAssignedVariable();
        inline bool hasNextAssignedVariable() const;
        inline void startIterationOnAssignedVariable();

        inline unsigned int getCurrentDecisionLevel();
        inline void incrementCurrentDecisionLevel();
        
        inline void assignLiteral( Literal literal );
        inline void assignLiteral( Clause* implicant );
        inline void assignLiteral( Literal literal, Reason* implicant );
        
        inline bool propagateLiteralAsDeterministicConsequence( Literal literal );
        inline bool propagateLiteralAsDeterministicConsequenceSatelite( Literal literal );
        
        inline void onLearningClause( Literal literalToPropagate, Clause* learnedClause, unsigned int backjumpingLevel );
        inline void onLearningUnaryClause( Literal literalToPropagate, Clause* learnedClause );        
        inline bool doRestart();
        inline void unrollToZero();
        
        inline unsigned int numberOfClauses() const { return clauses.size(); }
        inline unsigned int numberOfLearnedClauses() const;         
        inline unsigned int numberOfAssignedLiterals() const;
        inline unsigned int numberOfVariables() const;
        
        inline Var getAssignedVariable( unsigned idx ) { return variables.getAssignedVariable( idx ); }
        
        inline void setAChoice( Literal choice );        
        
        inline bool analyzeConflict();
        inline void clearConflictStatus();
        inline bool performAssumptions( vector< Literal >& assumptions );
        inline bool chooseLiteral( vector< Literal >& assumptions );
        inline bool conflictDetected();
        inline void foundIncoherence();
        inline bool hasUndefinedLiterals();
        inline void printAnswerSet();
        inline void printOptimizationValue( const Vector< uint64_t >& costs );
        inline void printCautiousConsequences( const Vector< Var >& answers );        
        inline void optimumFound();
        
        void unroll( unsigned int level );
        inline void unrollOne();
        inline void unrollLastVariable();
        
        /* OPTIONS */
        inline void setOutputBuilder( OutputBuilder* value );        
        
        typedef vector< Clause* >::iterator ClauseIterator;
        typedef vector< Clause* >::reverse_iterator ClauseReverseIterator;
        typedef vector< Clause* >::const_iterator ConstClauseIterator;
        typedef vector< Clause* >::const_reverse_iterator ConstClauseReverseIterator;
        
        inline Clause* clauseAt( unsigned int i ) { assert( i < numberOfClauses() ); return clauses[ i ]; }
        
        inline ClauseIterator clauses_begin() { return clauses.begin(); }
        inline ClauseIterator clauses_end() { return clauses.end(); }
        inline ClauseReverseIterator clauses_rbegin() { return clauses.rbegin(); }
        inline ClauseReverseIterator clauses_rend() { return clauses.rend(); }
        inline ClauseIterator learnedClauses_begin() { return learnedClauses.begin(); }
        inline ClauseIterator learnedClauses_end() { return learnedClauses.end(); }
        inline ClauseReverseIterator learnedClauses_rbegin() { return learnedClauses.rbegin(); }
        inline ClauseReverseIterator learnedClauses_rend() { return learnedClauses.rend(); }
        inline ConstClauseIterator clauses_begin() const { return clauses.begin(); }
        inline ConstClauseIterator clauses_end() const { return clauses.end(); }
        inline ConstClauseReverseIterator clauses_rbegin() const { return clauses.rbegin(); }
        inline ConstClauseReverseIterator clauses_rend() const { return clauses.rend(); }
        inline ConstClauseIterator learnedClauses_begin() const { return learnedClauses.begin(); }
        inline ConstClauseIterator learnedClauses_end() const { return learnedClauses.end(); }
        inline ConstClauseReverseIterator learnedClauses_rbegin() const { return learnedClauses.rbegin(); }
        inline ConstClauseReverseIterator learnedClauses_rend() const { return learnedClauses.rend(); }

        inline void deleteLearnedClause( ClauseIterator iterator );
        inline void deleteClause( Clause* clause );
        inline void removeClauseNoDeletion( Clause* clause );
        inline void deleteClauses() { glucoseHeuristic_ ? glucoseDeletion() : minisatDeletion(); }
        void minisatDeletion();
        void glucoseDeletion();
        inline void decrementActivity(){ deletionCounters.increment *= deletionCounters.decrement; }
        inline void onLearning( Clause* learnedClause );        
        inline void markClauseForDeletion( Clause* clause ){ satelite->onDeletingClause( clause ); clause->markAsDeleted(); }
        
        inline void deleteClausesIfNecessary();
        inline bool restartIfNecessary();
        
        void printProgram() const;
        void printDimacs() const;
        
//        inline void initClauseData( Clause* clause ) { assert( heuristic != NULL ); heuristic->initClauseData( clause ); }
//        inline Heuristic* getHeuristic() { return heuristic; }
        inline void onLiteralInvolvedInConflict( Literal l ) { minisatHeuristic->onLiteralInvolvedInConflict( l ); }
        inline void finalizeDeletion( unsigned int newVectorSize ) { learnedClauses.resize( newVectorSize ); }        
        
        inline void setRestart( Restart* r );
        
        void simplifyOnRestart();
        void removeSatisfied( vector< Clause* >& clauses );

        inline void onEliminatingVariable( Var variable, unsigned int sign, Clause* definition );
        inline void completeModel();
        
        inline Clause* newClause( unsigned reserve = 8 );
        inline void releaseClause( Clause* clause );
        
        inline void addPostPropagator( PostPropagator* postPropagator );
        inline void resetPostPropagators();
        
        inline void addEdgeInDependencyGraph( unsigned int v1, unsigned int v2 ){ trace_msg( parser, 10, "Add arc " << v1 << " -> " << v2 ); dependencyGraph->addEdge( v1, v2 ); }
        inline void computeStrongConnectedComponents();                        
        
        void createCyclicComponents();
        inline void addHCComponent( HCComponent* c ) { hcComponents.push_back( c ); }

        inline bool tight() const { return cyclicComponents.empty() && hcComponents.empty(); }
        inline unsigned int getNumberOfCyclicComponents() const { return cyclicComponents.size(); }
        inline Component* getCyclicComponent( unsigned int position ) { return cyclicComponents[ position ]; }
        
        inline void addGUSData( GUSData* gd ) { gusDataVector.push_back( gd ); }        
        
        inline void onStrengtheningClause( Clause* clause ) { satelite->onStrengtheningClause( clause ); }
        
        inline Satelite* getSatelite() { return satelite; }
        
        inline void addAggregate( Aggregate* aggr ) { assert( aggr != NULL ); aggregates.push_back( aggr ); }
        inline bool hasPropagators() const { return ( !tight() || !aggregates.empty() ); }
        
        inline void turnOffSimplifications() { callSimplifications_ = false; }
        inline bool callSimplifications() const { return callSimplifications_; }
        
//        inline void setOptimizationAggregate( Aggregate* optAggregate ) { assert( optAggregate != NULL ); optimizationAggregate = optAggregate; }
        inline void addOptimizationLiteral( Literal lit, uint64_t weight, unsigned int level, bool isAux );
        inline uint64_t computeCostOfModel( unsigned int level );
//        inline uint64_t computeCostOfModel();
        inline void computeCostOfModel( Vector< uint64_t >& );
        inline void setLevels( unsigned int n );
//        inline bool updateOptimizationAggregate( unsigned int modelCost );        
        
//        inline uint64_t getCostOfLevel( unsigned int levelIndex, uint64_t totalCost ) const;
//        inline void setMaxCostOfLevelOfOptimizationRules( vector< uint64_t >& m ) { maxCostOfLevelOfOptimizationRules.swap( m ); }
//        inline void setNumberOfOptimizationLevels( unsigned int n ) { numberOfOptimizationLevels = n; }        
        inline void addPreferredChoicesFromOptimizationLiterals( unsigned int level );
        inline void removePrefChoices() { minisatHeuristic->removePrefChoices(); }
        
        inline bool isTrue( Var v ) const { return variables.isTrue( v ); }
        inline bool isFalse( Var v ) const { return variables.isFalse( v ); }        
        inline bool isUndefined( Var v ) const { return variables.isUndefined( v ); }
        
        inline bool isTrue( Literal lit ) const { return variables.isTrue( lit ); }
        inline bool isFalse( Literal lit ) const { return variables.isFalse( lit ); }
        inline bool isUndefined( Literal lit ) const { return variables.isUndefined( lit ); }
        
        inline bool setTruthValue( Var v, TruthValue truth ) { return variables.setTruthValue( v, truth ); }
        inline void setUndefined( Var v ) { variables.setUndefined( v ); }
        inline void setUndefinedBrutal( Var v ) { variables.setUndefinedBrutal( v ); }
        
        inline bool setTrue( Literal lit ) { return variables.setTrue( lit ); }

        inline bool isImplicant( Var v, const Clause* clause ) const { return variables.isImplicant( v, clause ); }
        inline bool hasImplicant( Var v ) const { return variables.hasImplicant( v ); }
        inline void setImplicant( Var v, Reason* clause ) { variables.setImplicant( v, clause ); }
        inline Reason* getImplicant( Var v ) { return variables.getImplicant( v ); }

        inline unsigned int getDecisionLevel( Var v ) const { return variables.getDecisionLevel( v ); }
        inline unsigned int getDecisionLevel( Literal lit ) const { return getDecisionLevel( lit.getVariable() ); }
        inline void setDecisionLevel( Var v, unsigned int decisionLevel ) { variables.setDecisionLevel( v, decisionLevel ); }

        inline TruthValue getTruthValue( Var v ) const { return variables.getTruthValue( v ); }
        inline TruthValue getCachedTruthValue( Var v ) const { return variables.getCachedTruthValue( v ); }
        
        inline void addWatchedClause( Literal lit, Clause* clause ) { getDataStructure( lit ).variableWatchedLists.add( clause ); }
        inline void findAndEraseWatchedClause( Literal lit, Clause* clause ) { getDataStructure( lit ).variableWatchedLists.findAndRemove( clause ); }
        
        inline void addClause( Literal lit, Clause* clause ) { getDataStructure( lit ).variableAllOccurrences.push_back( clause ); }
        inline void findAndEraseClause( Literal lit, Clause* clause ) { getDataStructure( lit ).variableAllOccurrences.findAndRemove( clause ); }

        inline Clause* getOccurrence( Literal lit, unsigned index ) { return getDataStructure( lit ).variableAllOccurrences[ index ]; }
        inline unsigned int numberOfOccurrences( Literal lit ) const { return getDataStructure( lit ).variableAllOccurrences.size(); }
        inline unsigned int numberOfOccurrences( Var v ) const { Literal pos( v, POSITIVE ); return getDataStructure( pos ).variableAllOccurrences.size() + getDataStructure( pos.getOppositeLiteral() ).variableAllOccurrences.size(); }
        
        inline const Clause* getDefinition( Var v ) const { return variables.getDefinition( v ); }
//        inline void setEliminated( Var v, Clause* definition ) { variablesData[ v ].definition = definition; }
        inline unsigned int getSignOfEliminatedVariable( Var v ) const { return variables.getSignOfEliminatedVariable( v ); }
        inline bool hasBeenEliminated( Var v ) const { return variables.hasBeenEliminated( v ); }
        
        inline bool inTheSameComponent( Var v1, Var v2 ) const { return variables.inTheSameComponent( v1, v2 ); } 
        inline bool isInCyclicComponent( Var v ) const { return variables.isInCyclicComponent( v ); }
        inline void setComponent( Var v, Component* c ){ variables.setComponent( v, c ); }
        inline Component* getComponent( Var v ) { return variables.getComponent( v ); }
        
        inline bool inTheSameHCComponent( Var v1, Var v2 ) const { return variables.inTheSameHCComponent( v1, v2 ); } 
        inline bool isInCyclicHCComponent( Var v ) const { return variables.isInCyclicHCComponent( v ); }
        inline void setHCComponent( Var v, HCComponent* c ){ variables.setHCComponent( v, c ); }
        inline HCComponent* getHCComponent( Var v ) { return variables.getHCComponent( v ); }
        
        inline void addPropagator( Literal lit, Propagator* p, int position ) { getDataStructure( lit ).variablePropagators.push_back( pair< Propagator*, int >( p, position ) ); }
        inline void addPostPropagator( Literal lit, PostPropagator* p ) { getDataStructure( lit ).variablePostPropagators.push_back( p ); }
                
        bool isFrozen( Var v ) const { return variables.isFrozen( v ); }
        void setFrozen( Var v ) { variables.setFrozen( v ); }
        
        inline unsigned int cost( Var v ) const { Literal pos( v, POSITIVE ); return numberOfOccurrences( pos ) * numberOfOccurrences( pos.getOppositeLiteral() ); }
        
        void checkSubsumptionForClause( Clause* clause, Literal lit );
        bool isSubsumed( Clause* clause );
        
        Literal createFromAssignedVariable( Var v ) const { assert( TRUE == 2 && FALSE == 1 ); return Literal( v, getTruthValue( v ) & 1 ); }
        Literal createOppositeFromAssignedVariable( Var v ) const { assert( TRUE == 2 && FALSE == 1 ); return Literal( v, ~( getTruthValue( v ) ) & 1 ); }
        
        inline void clearOccurrences( Var v );     
        
        inline void attachClause( Clause& );
        inline void attachSecondWatch( Clause& );
//        inline void attachClause( unsigned int firstWatch, unsigned int secondWatch );
        inline void attachClauseToAllLiterals( Clause& );
        inline void detachClause( Clause& );
        inline void detachClauseFromAllLiterals( Clause& );
        inline void detachClauseFromAllLiterals( Clause&, Literal literal );
        
        inline void attachAggregate( Aggregate& );
        
        inline bool isSatisfied( const Clause& clause ) const;
        inline bool allUndefined( const Clause& clause ) const;
        
        inline bool isLocked( const Clause& clause ) const;
        
        inline void markAllClauses( Var v );
        inline void removeAllClauses( Var v );
        inline void removeLiteralAndMarkClause( Clause& clause, Literal literal );
        inline bool removeSatisfiedLiterals( Clause& clause );
        
        inline Literal getLiteralWithMinOccurrences( const Clause& clause ) const;
        inline Var getVariableWithMinOccurrences( const Clause& clause ) const;
        
        inline bool onLiteralFalse( Clause& clause, Literal literal );
        
        inline const DataStructures& getDataStructure( Literal lit ) const { return *variableDataStructures[ lit.getIndex() ]; }
        inline DataStructures& getDataStructure( Literal lit ) { return *variableDataStructures[ lit.getIndex() ]; }        

        inline void learnedClauseUsedForConflict( Clause* clause );
        inline unsigned int computeLBD( const Clause& clause );
        
        inline void bumpActivity( Var v ) { minisatHeuristic->bumpActivity( v ); }
        
        inline bool glucoseHeuristic() const { return glucoseHeuristic_; }
        inline void disableGlucoseHeuristic() { glucoseHeuristic_ = false; }
        inline bool minimisationWithBinaryResolution( Clause& learnedClause, unsigned int lbd );
        
        inline bool modelIsValidUnderAssumptions( vector< Literal >& assumptions );
        
        void initFrom( Solver& solver );
        
        HCComponent* createHCComponent( unsigned numberOfInputAtoms );
        
        void printLearnedClauses();
        
        bool cleanAndAddLearnedClause( Clause* c );
        inline void setExchangeClauses( bool ex ) { exchangeClauses_ = ex; }
        inline bool exchangeClauses() const { return exchangeClauses_; }
        inline void setGenerator( bool gen ) { generator = gen; statistics( this, setGenerator( gen ) ); }        
        inline void setAfterConflictPropagator( PostPropagator* p ) { assert( afterConflictPropagator == NULL ); afterConflictPropagator = p; }
        
        static void addClauseInLearnedFromAllSolvers( Clause* c ) { learnedFromAllSolvers.push_back( c ); }
        
        inline void disableStatistics() { statistics( this, disable() ); }
        inline void enableStatistics() { statistics( this, enable() ); }
        
        inline unsigned int numberOfHCComponents() const { return hcComponents.size(); }
        
        inline void printInterpretation() const { variables.printInterpretation(); }
        inline void setHCComponentForChecker( HCComponent* hc ) { assert( hcComponentForChecker == NULL ); hcComponentForChecker = hc; }
        
        inline void onLearningALoopFormulaFromModelChecker() { learnedFromPropagators++; }
        inline void onLearningALoopFormulaFromGus() { learnedFromConflicts++; }
        
        inline void disableVariableElimination() { assert( satelite != NULL ); satelite->disableVariableElimination(); }

        inline void setMinisatHeuristic() { glucoseHeuristic_ = false; }
        
        void clearAfterSolveUnderAssumptions( const vector< Literal >& assumptions );
        
        inline void setAssumption( Literal lit, bool isAssumption ) { variables.setAssumption( lit.getVariable(), isAssumption ); }
        inline bool isAssumption( Var v ) const { return variables.isAssumption( v ); }
        
        inline void computeUnsatCore();
        inline void minimizeUnsatCore( vector< Literal >& assumptions );
        inline void setMinimizeUnsatCore( bool b ) { minimizeUnsatCore_ = b; }
        inline void setComputeUnsatCores( bool b ) { computeUnsatCores_ = b; }
        inline const Clause* getUnsatCore() const { return unsatCore; }
        
        inline OptimizationLiteralData& getOptimizationLiteral( unsigned int level, unsigned int pos );
        inline unsigned int numberOfOptimizationLiterals( unsigned int level ) const { assert( level < optimizationLiterals.size() ); return optimizationLiterals[ level ].size(); }
        inline unsigned int numberOfLevels() const { return optimizationLiterals.size(); }
        void sortOptimizationLiterals( unsigned int level );
        
//        inline Aggregate* createAggregate( const vector< Literal >& literals, const vector< unsigned int >& weights );
        
        inline bool isWeighted( unsigned int level ) const { assert( level < weighted_.size() ); return weighted_[ level ]; }
        inline void setWeighted( unsigned int level ) { assert( level < weighted_.size() ); weighted_[ level ] = true; }
        
        inline void setMaxNumberOfChoices( unsigned int max ) { maxNumberOfChoices = max; }
        inline void setMaxNumberOfRestarts( unsigned int max ) { maxNumberOfRestarts = max; }
        inline void setMaxNumberOfSeconds( unsigned int max ) { maxNumberOfSeconds = max; }
        inline uint64_t getPrecomputedCost( unsigned int level ) const { assert( level < precomputedCosts.size() ); return precomputedCosts[ level ]; }
//        inline uint64_t getPrecomputedCost() const { return precomputedCost; }                
        
        inline void foundLowerBound( uint64_t lb ) { outputBuilder->foundLowerBound( lb ); }
        inline bool incremental() const { return incremental_; }
        
        inline bool isOptimizationProblem() const { return !optimizationLiterals.empty(); }
        
        inline bool propagateFixpoint();
        
        inline uint64_t simplifyOptimizationLiterals( unsigned int level );
//        inline void simplifyOptimizationLiterals();
               
    private:
        HCComponent* hcComponentForChecker;
        PostPropagator* afterConflictPropagator;
        bool exchangeClauses_;
        bool generator;
        static vector< Clause* > learnedFromAllSolvers;

        unsigned int solveWithoutPropagators( vector< Literal >& assumptions );
        unsigned int solvePropagators( vector< Literal >& assumptions );

        void updateActivity( Clause* learnedClause );
        inline void addVariableInternal();        
        inline void addBinaryClause( Literal lit1, Literal lit2 );
        inline void addLiteralInShortClause( Literal firstLiteral, Literal secondLiteral ) { getDataStructure( firstLiteral ).variableBinaryClauses.push_back( secondLiteral ); }
        
        bool checkVariablesState();
        inline void setEliminated( Var v, unsigned int value, Clause* definition ) { variables.setEliminated( v, value, definition ); }        
        
        Solver( const Solver& ) : learning( *this ), dependencyGraph( NULL )
        {
//            dependencyGraph = new DependencyGraph( *this );
            assert( "The copy constructor has been disabled." && 0 );
        }

        unsigned int currentDecisionLevel;
        Variables variables;
        
        vector< Clause* > clauses;
        vector< Clause* > learnedClauses;
        
        vector< unsigned int > unrollVector;
        
        Literal conflictLiteral;        
        Reason* conflictClause;
        
        Learning learning;
        OutputBuilder* outputBuilder;        
        
        MinisatHeuristic* minisatHeuristic;
        Restart* restart;
        Satelite* satelite;                
        
        unsigned int getNumberOfUndefined() const;
        bool allClausesSatisfied() const;
        
        unsigned int assignedVariablesAtLevelZero;
        int64_t nextValueOfPropagation;
        
        uint64_t literalsInClauses;
        uint64_t literalsInLearnedClauses;
        
        vector< Var > eliminatedVariables;
//        vector< Clause* > poolOfClauses;

        DependencyGraph* dependencyGraph;
        Vector< PostPropagator* > postPropagators;
        Vector< Propagator* > propagatorsForUnroll;
        Vector< unsigned > fromLevelToPropagators;
        
        inline void addInPropagatorsForUnroll( Propagator* prop );
        
        vector< GUSData* > gusDataVector;
        vector< Aggregate* > aggregates;
        
//        Aggregate* optimizationAggregate;
//        unsigned int numberOfOptimizationLevels;
        vector< uint64_t > precomputedCosts;
        uint64_t precomputedCost;
                
        bool callSimplifications_;
        
        bool glucoseHeuristic_;
        uint64_t conflicts;
        uint64_t conflictsRestarts;                
        
        struct DeletionCounters
        {
            Activity increment;
            Activity decrement;

            double learnedSizeFactor;
            double learnedSizeIncrement;
            double maxLearned;
            unsigned int learnedSizeAdjustStartConfl;
            double learnedSizeAdjustConfl;
            double learnedSizeAdjustIncrement;
            unsigned int learnedSizeAdjustCnt;
            
            void init()
            {
                increment = 1 ;
                decrement = 1/0.999;
                learnedSizeFactor = ( ( double ) 1 / ( double) 3 );
                learnedSizeIncrement = 1.1;
                maxLearned = 0.0;
                learnedSizeAdjustStartConfl = 100;
                learnedSizeAdjustConfl = 0.0;
                learnedSizeAdjustCnt = 0;
                learnedSizeAdjustIncrement = 1.5;
            }
        } deletionCounters;
        
        struct GlucoseData
        {
            double sizeLBDQueue;
            double sizeTrailQueue;
            double K;
            double R;
            
            //constants for reduce DB
            int nbclausesBeforeReduce;
            int incReduceDB;
            int specialIncReduceDB;
            unsigned int lbLBDFrozenClause;
            
            //constants for reducing clause
            int lbSizeMinimizingClause;
            unsigned int lbLBDMinimizingClause;
            
            float sumLBD;
            
            Vector< unsigned int > permDiff;
            
            unsigned int currRestart;
            
            unsigned int MYFLAG;
            
            bqueue< unsigned int > lbdQueue;
            bqueue< unsigned int > trailQueue;            
            
            void init()
            {
                sizeLBDQueue = 50;
                sizeTrailQueue = 5000;
                K = 0.8;
                R = 1.4;
                nbclausesBeforeReduce = 2000;
                incReduceDB = 300;
                specialIncReduceDB = 1000;
                lbLBDFrozenClause = 30;
                
                lbSizeMinimizingClause = 30;
                lbLBDMinimizingClause = 6;
                
                sumLBD = 0.0;
                currRestart = 1;
                
                permDiff.push_back( 0 );
                MYFLAG = 0;
                
                lbdQueue.initSize( sizeLBDQueue );
                trailQueue.initSize( sizeTrailQueue );
            }
            
            void onNewVariable() { permDiff.push_back( 0 ); }
            
        } glucoseData;
        
        vector< vector< OptimizationLiteralData* > > optimizationLiterals;
        vector< bool > weighted_;
//        vector< uint64_t > maxCostOfLevelOfOptimizationRules;        
        
        Vector< DataStructures* > variableDataStructures;
        
        vector< Component* > cyclicComponents;
        vector< HCComponent* > hcComponents;
        
        unsigned int numberOfAssumptions;
        unsigned int learnedFromPropagators;
        unsigned int learnedFromConflicts;                
        bool partialChecks;
        bool computeUnsatCores_;
        bool minimizeUnsatCore_;
        Clause* unsatCore;        
        unsigned int maxNumberOfChoices;
        unsigned int numberOfChoices;
        unsigned int maxNumberOfRestarts;
        unsigned int numberOfRestarts;
        unsigned int maxNumberOfSeconds;
        
        bool incremental_;
        
        #ifndef NDEBUG
        bool checkStatusBeforePropagation( Var variable )
        {
            unsigned int sign = ( getTruthValue( variable ) >> 1 );
            assert_msg( sign <= 1, "Sign " << sign << " should be less or equal to 1" );
            assert_msg( getTruthValue( variable ) == TRUE ? sign == NEGATIVE : sign == POSITIVE, "TruthValue of variable does not correspond to the sign" );
            assert_msg( !isUndefined( variable ), "Propagating variable " << this << ", the truth value is Undefined." );
            assert( FALSE == 1 && TRUE == 2 );
            
            return true;
        }
        #endif
};

Solver::Solver() 
:    
    hcComponentForChecker( NULL ),
    afterConflictPropagator( NULL ),
    exchangeClauses_( false ),
    generator( true ),
    currentDecisionLevel( 0 ),
    conflictLiteral( Literal::null ),
    conflictClause( NULL ),
    learning( *this ),        
    outputBuilder( NULL ),
    restart( NULL ),
    assignedVariablesAtLevelZero( MAXUNSIGNEDINT ),
    nextValueOfPropagation( 0 ),
    literalsInClauses( 0 ),
    literalsInLearnedClauses( 0 ),
//    optimizationAggregate( NULL ),
//    numberOfOptimizationLevels( 0 ),
    precomputedCost( 0 ),
    callSimplifications_( true ),
    glucoseHeuristic_( true ),
    conflicts( 0 ),
    conflictsRestarts( 0 ),
    numberOfAssumptions( 0 ),
    learnedFromPropagators( 0 ),
    learnedFromConflicts( 0 ),
    partialChecks( true ),
    computeUnsatCores_( false ),
    minimizeUnsatCore_( true ),
    unsatCore( NULL ),
    maxNumberOfChoices( UINT_MAX ),
    numberOfChoices( 0 ),
    maxNumberOfRestarts( UINT_MAX ),
    numberOfRestarts( 0 ),
    maxNumberOfSeconds( UINT_MAX ),
    incremental_( false )
{
    dependencyGraph = new DependencyGraph( *this );
    satelite = new Satelite( *this );
    minisatHeuristic = new MinisatHeuristic( *this );
    deletionCounters.init();
    glucoseData.init();
    VariableNames::addVariable();
    variableDataStructures.push_back( NULL );
    variableDataStructures.push_back( NULL );
    fromLevelToPropagators.push_back( 0 );
}

void
Solver::onKill()
{
    if( outputBuilder )
        outputBuilder->onKill();    
}

unsigned int
Solver::solve()
{
    incremental_ = false;
    numberOfAssumptions = 0;    
    vector< Literal > assumptions;
    if( !hasPropagators() )
        return solveWithoutPropagators( assumptions );
    else
        return solvePropagators( assumptions );
}

unsigned int
Solver::solve(
    vector< Literal >& assumptions )
{
    incremental_ = !assumptions.empty();
    numberOfAssumptions = assumptions.size();
    for( unsigned int i = 0; i < assumptions.size(); i++ )
        setAssumption( assumptions[ i ], true );
        
    delete unsatCore;
    unsatCore = NULL;
    unsigned int result = ( !hasPropagators() ) ? solveWithoutPropagators( assumptions ) : solvePropagators( assumptions );
    if( computeUnsatCores_ && result == INCOHERENT )
    {        
        if( unsatCore == NULL )
            unsatCore = new Clause();
        else
        {
            if( minimizeUnsatCore_ )
                minimizeUnsatCore( assumptions );
        }
    }
    clearAfterSolveUnderAssumptions( assumptions );
    clearConflictStatus();
    return result;    
}

void
Solver::setOutputBuilder(
    OutputBuilder* value )
{
    assert( value != NULL );
    if( outputBuilder != NULL )
        delete outputBuilder;
    outputBuilder = value;
}

void
Solver::addVariableInternal()
{
    VariableNames::addVariable();
    variables.push_back();    
    minisatHeuristic->onNewVariable( variables.numberOfVariables() );
    learning.onNewVariable();
    glucoseData.onNewVariable();
    
    variableDataStructures.push_back( new DataStructures() );
    variableDataStructures.push_back( new DataStructures() );

//    return variables.numberOfVariables();
}

//void
//Solver::addVariable( 
//    const string& name )
//{    
//    Var variable = addVariableInternal();
//    VariableNames::setName( variable, name );    
//}

void
Solver::addVariable()
{
    addVariableInternal();
}

void
Solver::addVariableRuntime()
{
    addVariableInternal();
    minisatHeuristic->onNewVariableRuntime( numberOfVariables() );
}

Literal
Solver::getLiteral(
    int lit )
{
//    assert( "Lit is out of range." && static_cast< unsigned >( abs( lit ) ) <= variables.numberOfVariables() && abs( lit ) > 0);
    Var var = lit > 0 ? lit : -lit;
    return lit > 0 ? Literal( var, POSITIVE ) : Literal( var, NEGATIVE );
}

//Var
//Solver::getVariable(
//    unsigned int var )
//{
//    assert_msg( ( var > 0 && var <= variables.numberOfVariables() ), "Variable id " << var << " is greater than the number of variables: " << numberOfVariables() );
//    return variables[ var ];
//}

void
Solver::assignLiteral(
    Literal literal )
{
    assert( !conflictDetected() );
    if( !variables.assign( currentDecisionLevel, literal ) )
    {
        conflictLiteral = literal;
        conflictClause = NULL; 
    }
}

void
Solver::assignLiteral(
    Clause* implicant )
{
    assert( implicant != NULL );
    assert( !conflictDetected() );
    if( !variables.assign( currentDecisionLevel, implicant ) )
    {
        conflictLiteral = implicant->getAt( 0 );
        conflictClause = implicant;        
    }
}

void
Solver::assignLiteral(
    Literal lit,
    Reason* implicant )
{
    assert( implicant != NULL );
    
    assert( !conflictDetected() );
    if( !variables.assign( currentDecisionLevel, lit, implicant ) )
    {
        conflictLiteral = lit;
        conflictClause = implicant;        
    }
}

bool
Solver::cleanAndAddClause(
    Clause* clause )
{
    assert( clause != NULL );
    
    if( clause->removeDuplicatesAndFalseAndCheckIfTautological( *this ) )
    {
        trace_msg( solving, 10, "Found tautological clause: " << *clause );
        releaseClause( clause );
        return true;
    }
    
    if( clause->size() == 0 )
    {
        trace_msg( solving, 10, "Found contradictory (empty) clause" );
        conflictLiteral = Literal::conflict;
        releaseClause( clause );
        return false;
    }
    
    if( clause->size() == 2 )
    {
        trace_msg( solving, 10, "Replace by binary clause: " << *clause );
        Literal lit1 = clause->getAt( 0 );
        Literal lit2 = clause->getAt( 1 );
        releaseClause( clause );
        return addClause( lit1, lit2 );
    }
    
    trace_msg( solving, 10, "Clause after simplification: " << *clause );
    
    assert( allUndefined( *clause ) );
    return addClause( clause );
}

bool
Solver::addClause(
    Literal literal )
{
    if( isTrue( literal ) || propagateLiteralAsDeterministicConsequence( literal ) )
        return true;
    
    conflictLiteral = literal;
    return false;
}

bool
Solver::addClause(
    Literal lit1,
    Literal lit2 )
{    
    if( !callSimplifications() )
    {
        addBinaryClause( lit1, lit2 );
    }
    else
    {
        Clause* bin = newClause( 2 );
        bin->addLiteral( lit1 );
        bin->addLiteral( lit2 );
        return addClause( bin );
    }
    
    return true;
}

bool
Solver::addClauseRuntime(
    Literal literal )
{
    if( isTrue( literal ) )
        return true;
    if( isFalse( literal ) )
        return false;
    assert( currentDecisionLevel == 0 );
    assert( !conflictDetected() );
    assignLiteral( literal );
    assert( isTrue( literal ) );
    assert( !conflictDetected() );

    while( hasNextVariableToPropagate() )
    {
        nextValueOfPropagation--;
        Var variableToPropagate = getNextVariableToPropagate();
        if( hasPropagators() )
            propagateWithPropagators( variableToPropagate );
        else
            propagate( variableToPropagate );

        if( conflictDetected() )
            return false;
    }
    return true;
}

void
Solver::addBinaryClauseRuntime(
    Clause* clause )
{
    assert( !callSimplifications_ );
    addBinaryClause( clause->getAt( 0 ), clause->getAt( 1 ) );
    releaseClause( clause );
}

bool
Solver::addClauseRuntime(
    Clause* clausePointer )
{
    assert( clausePointer != NULL );
    Clause& clause = *clausePointer;
    assert( allUndefined( clause ) );
    assert( !clause.isTautology() );
    assert( currentDecisionLevel == 0 );
    unsigned int size = clause.size();
    switch( size )
    {
        case 0:
            releaseClause( clausePointer );
            return false;
        
        case 1:
        {
            Literal tmpLit = clause[ 0 ];
            releaseClause( clausePointer );
            return addClauseRuntime( tmpLit );
        }
        case 2:
            addBinaryClauseRuntime( clausePointer );
            return true;
            
        default:
            attachClause( clause );
            clauses.push_back( clausePointer );
            return true;
    }

    return true;
}

void
Solver::addBinaryClause(
    Literal lit1,
    Literal lit2 )
{
//    Vector< Literal >& lit1BinClauses = getDataStructure( lit1 ).variableBinaryClauses;
//    Vector< Literal >& lit2BinClauses = getDataStructure( lit2 ).variableBinaryClauses;
//    
//    if( lit1BinClauses.size() < lit2BinClauses.size() )
//    {
//        if( lit1BinClauses.findElement( lit2 ) != MAXUNSIGNEDINT )
//        {
//            assert( lit2BinClauses.findElement( lit1 ) != MAXUNSIGNEDINT );
//            return;
//        }
//    }
//    else
//    {
//        if( lit2BinClauses.findElement( lit1 ) != MAXUNSIGNEDINT )
//        {
//            assert( lit1BinClauses.findElement( lit2 ) != MAXUNSIGNEDINT );
//            return;
//        }
//    }
    addLiteralInShortClause( lit1, lit2 );
    addLiteralInShortClause( lit2, lit1 );
}

bool
Solver::addClause(
    Clause* clause )
{
    assert( clause != NULL );
    assert( allUndefined( *clause ) );
    assert( !clause->isTautology() );
    
    unsigned int size = clause->size();    
    if( size > 1 )
    {        
        statistics( this, onAddingClause( size ) );
        if( callSimplifications() || clause->size() != 2 )
            attachClauseToAllLiterals( *clause );
        clause->setPositionInSolver( clauses.size() );
        clauses.push_back( clause );
        return true;
    }        

    if( size == 1 )
    {
        if( addClause( clause->getAt( 0 ) ) )
        {
            releaseClause( clause );
            return true;
        }
    }

    conflictLiteral = Literal::conflict;
    releaseClause( clause );
    return false;
}

bool
Solver::addClauseFromModel(
    Clause* clause )
{
    assert( clause != NULL );
    unsigned int size = clause->size();
    assert( size != 0 );
    
    if( size > 1 )
    {
        statistics( this, onAddingClause( size ) );
        attachClause( *clause );
        clause->setPositionInSolver( clauses.size() );
        clauses.push_back( clause );
        return true;
    }
    else
    {
        assert( !conflictDetected() );
        assignLiteral( clause->getAt( 0 ) );
        releaseClause( clause );
    }

    return true;
}

Var
Solver::getNextVariableToPropagate()
{
    assert( variables.hasNextVariableToPropagate() );
    return variables.getNextVariableToPropagate();
}
        
bool
Solver::hasNextVariableToPropagate() const
{
    return variables.hasNextVariableToPropagate();
}

unsigned int
Solver::getCurrentDecisionLevel()
{
    return currentDecisionLevel;
}

void
Solver::incrementCurrentDecisionLevel()
{
    currentDecisionLevel++;
    unrollVector.push_back( variables.numberOfAssignedLiterals() );
    
    assert( currentDecisionLevel == unrollVector.size() );
}

void
Solver::unrollLastVariable()
{    
    minisatHeuristic->onUnrollingVariable( variables.unrollLastVariable() );
}

void
Solver::unrollOne()
{
    unroll( currentDecisionLevel - 1 );
}

bool
Solver::doRestart()
{
    assert( currentDecisionLevel != 0 );
    trace( solving, 2, "Performing restart.\n" );
    numberOfRestarts++;
    restart->onRestart();
    
    assert( incremental_ || numberOfAssumptions == 0 );
    if( currentDecisionLevel > numberOfAssumptions )
        unroll( numberOfAssumptions );
    else
        unroll( 0 );
        

    if( generator && exchangeClauses_ )
    {
        while( !learnedFromAllSolvers.empty() )
        {
            Clause* c = learnedFromAllSolvers.back();
            learnedFromAllSolvers.pop_back();            
            if( !cleanAndAddLearnedClause( c ) )
                return false;
        }
    }
    return true;
}

void
Solver::unrollToZero()
{
    if( currentDecisionLevel != 0 )    
        unroll( 0 );
}

void
Solver::deleteLearnedClause( 
    ClauseIterator iterator )
{
    Clause* learnedClause = *iterator;
    trace_msg( solving, 4, "Deleting learned clause " << *learnedClause );
    detachClause( *learnedClause );
    literalsInLearnedClauses -= learnedClause->size();
    releaseClause( learnedClause );
//    delete learnedClause;
//    learnedClauses.erase( iterator );
}

void
Solver::deleteClause( 
    Clause* clause )
{
    unsigned int position = clause->getPositionInSolver();
    assert_msg( position < clauses.size(), "Position " << position << " is greater than size of the clauses vector " << clauses.size() );
    assert_msg( clause == clauses[ position ], "The clause to delete " << *clause << " is not equal to the clause " << clauses[ position ] << " in position " << position  );
    trace_msg( solving, 4, "Deleting clause " << *clause );

    clauses[ position ] = clauses.back();
    trace_msg( solving, 6, "Swapping clause " << *clause << " and " << *clauses[ position ] );
    clauses[ position ]->setPositionInSolver( position );
    clauses.pop_back();
//    delete clause;
    releaseClause( clause );
}

void
Solver::removeClauseNoDeletion( 
    Clause* clause )
{
    satelite->onDeletingClause( clause );
    unsigned int position = clause->getPositionInSolver();
    assert_msg( position < clauses.size(), "Position " << position << " is greater than size of the clauses vector " << clauses.size() );
    assert_msg( clause == clauses[ position ], "The clause to delete " << *clause << " is not equal to the clause " << clauses[ position ] << " in position " << position  );
    trace_msg( solving, 4, "Deleting clause " << *clause );

    clauses[ position ] = clauses.back();
    trace_msg( solving, 6, "Swapping clause " << *clause << " and " << *clauses[ position ] );
    clauses[ position ]->setPositionInSolver( position );
    clauses.pop_back();
}

unsigned int
Solver::numberOfLearnedClauses() const
{
    return learnedClauses.size();
}

bool
Solver::conflictDetected()
{
    return conflictLiteral != Literal::null;
}

bool
Solver::hasUndefinedLiterals()
{
    return variables.numberOfAssignedLiterals() < variables.numberOfVariables();
}

void
Solver::printAnswerSet()
{
    variables.printAnswerSet( outputBuilder );
}

void
Solver::printOptimizationValue(
    const Vector< uint64_t >& costs )
{
    outputBuilder->foundModelOptimization( costs );
}

void
Solver::printCautiousConsequences(
    const Vector< Var >& answers )
{
    outputBuilder->startModel();
    for( unsigned int i = 0; i < answers.size(); ++i )
        outputBuilder->printVariable( answers[ i ], true );
    outputBuilder->endModel();
}

void
Solver::optimumFound()
{
    outputBuilder->optimumFound();
}

void
Solver::foundIncoherence()
{
    outputBuilder->onProgramIncoherent();
}

bool
Solver::chooseLiteral(
    vector< Literal >& assumptions )
{        
    Literal choice = Literal::null;
    for( unsigned int i = currentDecisionLevel; i < assumptions.size(); i++ )
    {
        if( isUndefined( assumptions[ i ] ) )
        {
            if( choice == Literal::null )
                choice = assumptions[ i ];
        }
        else if( isFalse( assumptions[ i ] ) )
        {
            conflictLiteral = assumptions[ i ];
            trace_msg( solving, 1, "The assumption " << assumptions[ i ] << " is false: stop" );
            if( computeUnsatCores_ )    
            {
                assert( unsatCore == NULL );                 
                computeUnsatCore();                
            }
            return false;
        }
        else
        {
            if( choice == Literal::null )
                incrementCurrentDecisionLevel();
        }
    }
    
    if( choice != Literal::null )
        goto end;    
    choice = minisatHeuristic->makeAChoice();    
    
    end:;
    trace_msg( solving, 1, "Choice: " << choice );
    setAChoice( choice );    
    statistics( this, onChoice() );    
    return true;
}

bool
Solver::analyzeConflict()
{
    conflicts++;
    conflictsRestarts++;
    
    if( glucoseHeuristic_ )
    {
        glucoseData.trailQueue.push( numberOfAssignedLiterals() );
        if( conflictsRestarts > 10000 && glucoseData.lbdQueue.isValid() && numberOfAssignedLiterals() /*trail.size()*/ > glucoseData.R * glucoseData.trailQueue.getAvg() )
            glucoseData.lbdQueue.fastClear();
    }    

    Clause* learnedClause = learning.onConflict( conflictLiteral, conflictClause );
    assert( "Learned clause has not been calculated." && learnedClause != NULL );
    statistics( this, onLearning( learnedClause->size() ) );
    
    unsigned int size = learnedClause->size();
    
    if( size == 1 )
    {
//        if( !doRestart() )
//            return false;        
        unrollToZero();
        clearConflictStatus();
        Literal tmpLit = learnedClause->getAt( 0 );
        releaseClause( learnedClause );
        if( !addClauseRuntime( tmpLit ) )
            return false;
            
//        assignLiteral( learnedClause->getAt( 0 ) );
//        assert( isTrue( learnedClause->getAt( 0 ) ) );
//        assert( !conflictDetected() );
//        delete learnedClause;        

//        while( hasNextVariableToPropagate() )
//        {
//            nextValueOfPropagation--;            
//            Var variableToPropagate = getNextVariableToPropagate();
//            if( hasPropagators() )
//                propagateWithPropagators( variableToPropagate );
//            else
//                propagate( variableToPropagate );
//
//            if( conflictDetected() )
//                return false;
//        }
        
        simplifyOnRestart();
    }
    else
    {
        if( glucoseHeuristic_ )
        {
            glucoseData.sumLBD += learnedClause->lbd();
            glucoseData.lbdQueue.push( learnedClause->lbd() );
        }
        
        Literal firstLiteral = learnedClause->getAt( 0 );
        Literal secondLiteral = learnedClause->getAt( 1 );
        //Be careful. UIP should be always in position 0.
        assert( getDecisionLevel( firstLiteral ) == currentDecisionLevel );
        assert( getDecisionLevel( secondLiteral ) == learnedClause->getMaxDecisionLevel( *this, 1, learnedClause->size() ) );        
        
        unsigned int unrollLevel = getDecisionLevel( secondLiteral );
        assert_msg( unrollLevel != 0, "Trying to backjumping to level 0" );
        assert_msg( unrollLevel < currentDecisionLevel, "Trying to backjump from level " << unrollLevel << " to level " << currentDecisionLevel );
        trace_msg( solving, 2, "Learned clause and backjumping to level " << unrollLevel );
        addLearnedClause( learnedClause, true );        

        unroll( unrollLevel );
        clearConflictStatus();                        
        if( size != 2 )
        {
            assignLiteral( learnedClause );
            onLearning( learnedClause );  // FIXME: this should be moved outside
        }
        else
        {
            assignLiteral( firstLiteral, variables.getReasonForBinaryClauses( secondLiteral.getVariable() ) );
        }        

        clearConflictStatus();
    }

    if( --deletionCounters.learnedSizeAdjustCnt == 0 )
    {
        deletionCounters.learnedSizeAdjustConfl *= deletionCounters.learnedSizeAdjustIncrement;
        deletionCounters.learnedSizeAdjustCnt = ( unsigned int ) deletionCounters.learnedSizeAdjustConfl;
        deletionCounters.maxLearned *= deletionCounters.learnedSizeIncrement;
    }

    return true;
}

void
Solver::clearConflictStatus()
{
    resetPostPropagators();
    conflictLiteral = Literal::null;
    conflictClause = NULL;
}

unsigned int
Solver::numberOfAssignedLiterals() const
{
    return variables.numberOfAssignedLiterals();
}

unsigned int
Solver::numberOfVariables() const
{
    return variables.numberOfVariables();
}

void
Solver::setAChoice(
    Literal choice )
{
    assert( choice != Literal::null );
    incrementCurrentDecisionLevel();
    assert( isUndefined( choice ) );
    assignLiteral( choice );
}

Literal
Solver::getOppositeLiteralFromLastAssignedVariable()
{
    return variables.getOppositeLiteralFromLastAssignedVariable();
}

bool
Solver::hasNextAssignedVariable() const
{
    return variables.hasNextAssignedVariable();
}

void
Solver::startIterationOnAssignedVariable()
{
    variables.startIterationOnAssignedVariable();
}

bool
Solver::propagateLiteralAsDeterministicConsequence(
    Literal literal )
{
    assignLiteral( literal );
    if( conflictDetected() )
        return false;

    while( hasNextVariableToPropagate() )
    {
        Var variableToPropagate = getNextVariableToPropagate();
        propagateAtLevelZero( variableToPropagate );

        if( conflictDetected() )
            return false;
    }    
    assert( !conflictDetected() );

    return true;
}

bool
Solver::propagateLiteralAsDeterministicConsequenceSatelite(
    Literal literal )
{
    assignLiteral( literal );
    if( conflictDetected() )
        return false;

    while( hasNextVariableToPropagate() )
    {
        Var variableToPropagate = getNextVariableToPropagate();        
        propagateAtLevelZeroSatelite( variableToPropagate );

        if( conflictDetected() )
            return false;
    }    
    assert( !conflictDetected() );

    return true;
}

void
Solver::clearVariableOccurrences()
{
    for( unsigned int i = 1; i <= numberOfVariables(); i++ )
    {
        if( !variables.hasBeenEliminatedByDistribution( i ) )
            clearOccurrences( i );
        
        Literal pos( i, POSITIVE );
        Literal neg( i, NEGATIVE );
        if( !isUndefined( i ) )
        {
            getDataStructure( pos ).variableBinaryClauses.clearAndDelete();
            getDataStructure( neg ).variableBinaryClauses.clearAndDelete();
            continue;
        }        
        
        Vector< Literal >& posBinaryClauses = getDataStructure( pos ).variableBinaryClauses;
        
        unsigned int k = 0;
        for( unsigned int j = 0; j < posBinaryClauses.size(); ++j )
        {
            posBinaryClauses[ k ] = posBinaryClauses[ j ];
            assert( !isFalse( posBinaryClauses[ j ] ) );
            assert( isUndefined( posBinaryClauses[ j ] ) || getDecisionLevel( posBinaryClauses[ j ] ) == 0 );
            if( !isTrue( posBinaryClauses[ j ] ) )
                k++;
        }
        posBinaryClauses.shrink( k );        
   
        Vector< Literal >& negBinaryClauses = getDataStructure( neg ).variableBinaryClauses;
        k = 0;
        for( unsigned int j = 0; j < negBinaryClauses.size(); ++j )
        {
            negBinaryClauses[ k ] = negBinaryClauses[ j ];
            assert( !isFalse( negBinaryClauses[ j ] ) );
            assert( isUndefined( negBinaryClauses[ j ] ) || getDecisionLevel( negBinaryClauses[ j ] ) == 0 );
            if( !isTrue( negBinaryClauses[ j ] ) )
                k++;
        }
        negBinaryClauses.shrink( k );
        
    }
//    #ifndef NDEBUG    
//    for( unsigned int i = 1; i <= numberOfVariables(); i++ )
//        assert( getVariable( i )->hasBeenEliminated() || getVariable( i )->numberOfOccurrences() == 0 );    
//    #endif
}

void
Solver::attachWatches()
{
    for( unsigned int i = 0; i < clauses.size(); )
    {
        Clause* currentPointer = clauses[ i ];
        Clause& current = *currentPointer;
        if( current.hasBeenDeleted() )
        {
            deleteClause( currentPointer );
        }
        else
        {                    
            if( current.size() == 2 )
            {     
                addBinaryClause( current[ 0 ], current[ 1 ] );        
                deleteClause( currentPointer );
            }
            else
            {
                literalsInClauses += current.size();
                attachClause( current );
                ++i;
            }
        }
    }    
}

void
Solver::clearComponents()
{
    unsigned int j = 0;
    for( unsigned int i = 0; i < cyclicComponents.size(); i++ )
    {
        cyclicComponents[ j ] = cyclicComponents[ i ];
        cyclicComponents[ j ]->setId( j );
        if( !cyclicComponents[ i ]->isRemoved() )
            j++;
    }
    cyclicComponents.resize( j );
}

bool
Solver::preprocessing()
{
    if( conflictDetected() )
    {
        trace( solving, 1, "Conflict at level 0.\n" );
        return false;
    }    

    statistics( this, beforePreprocessing( numberOfVariables(), numberOfAssignedLiterals(), numberOfClauses() ) );
    assert( satelite != NULL );
    assert( checkVariablesState() );    
    if( callSimplifications() && !satelite->simplify() )
        return false;

    minisatHeuristic->simplifyVariablesAtLevelZero();
    clearVariableOccurrences();
    attachWatches();
    clearComponents();
    
    assignedVariablesAtLevelZero = numberOfAssignedLiterals();
    
    deletionCounters.maxLearned = numberOfClauses() * deletionCounters.learnedSizeFactor;
    deletionCounters.learnedSizeAdjustConfl = deletionCounters.learnedSizeAdjustStartConfl;
    deletionCounters.learnedSizeAdjustCnt = ( unsigned int ) deletionCounters.learnedSizeAdjustConfl;
    callSimplifications_ = false;
    statistics( this, afterPreprocessing( numberOfAssignedLiterals(), numberOfClauses() ) );

    return true;
}

void
Solver::onLearning(
    Clause* learnedClause )
{
    updateActivity( learnedClause );
    decrementActivity();    
}

void
Solver::setRestart(
    Restart* r )
{
    delete restart;    
    assert( r != NULL );    
    restart = r;
}

void
Solver::onEliminatingVariable(
    Var variable,
    unsigned int sign,
    Clause* definition )
{
    variables.onEliminatingVariable( variable );
    eliminatedVariables.push_back( variable );
    setEliminated( variable, sign, definition );
}

void
Solver::completeModel()
{
    trace_msg( solving, 1, "Completing the model for eliminated variables (" << eliminatedVariables.size() << " variables)" );
    for( int i = eliminatedVariables.size() - 1; i >= 0; i-- )
    {
        Var back = eliminatedVariables[ i ];
        trace_msg( satelite, 2, "Processing variable " << back );

        assert( hasBeenEliminated( back ) );
        unsigned int sign = getSignOfEliminatedVariable( back );
    
        if( sign == ELIMINATED_BY_DISTRIBUTION )
        {
            trace_msg( satelite, 3, "Eliminated by distribution " << back );
            bool found = false;            
            Literal positiveLiteral( back, POSITIVE );
            for( unsigned j = 0; j < numberOfOccurrences( positiveLiteral ); ++j )
            {
                Clause* clause = getOccurrence( positiveLiteral, j );
                assert( clause->hasBeenDeleted() );
                if( !isSatisfied( *clause ) )
                {
                    setUndefinedBrutal( back );
                    #ifndef NDEBUG
                    bool result =
                    #endif
                    setTrue( positiveLiteral );
                    assert( result );
                    found = true;
                    
                    trace_msg( satelite, 3, "Clause " << *clause << " is not satisfied: inferring " << positiveLiteral );
                    break;
                }
            }            
            if( !found )
            {
                Literal negativeLiteral( back, NEGATIVE );
                for( unsigned j = 0; j < numberOfOccurrences( negativeLiteral ); ++j )
                {
                    Clause* clause = getOccurrence( negativeLiteral, j );                    
                    assert( clause->hasBeenDeleted() );
                    if( !isSatisfied( *clause ) )
                    {
                        setUndefinedBrutal( back );

                        #ifndef NDEBUG
                        bool result =
                        #endif
                        setTrue( negativeLiteral );                            
                        assert( result );
                        
                        trace_msg( satelite, 3, "Clause " << *clause << " is not satisfied: inferring " << negativeLiteral );
                        break;
                    }
                }
            }
        }
        else
        {            
            assert( sign == POSITIVE || sign == NEGATIVE );
        
            Literal literal( back, sign );
            setUndefinedBrutal( back );
            const Clause* definition = getDefinition( back );
            trace_msg( satelite, 3, "Considering variable " << back << " and its definition " << *definition << " which is " << ( isSatisfied( *definition ) ? "satisfied" : "unsatisfied" ) );
            #ifndef NDEBUG
            bool result =
            #endif
            isSatisfied( *definition ) ? setTrue( literal.getOppositeLiteral() ) : setTrue( literal );                

            assert( result );
            trace_msg( satelite, 4, "Inferring " << ( isSatisfied( *definition ) ? literal.getOppositeLiteral() : literal ) );            
        }
    }
}

Clause*
Solver::newClause(
    unsigned reserve )
{
//    if( poolOfClauses.empty() )
//    {
//        unsigned int bufferSize = 20;
//        for( unsigned int i = 0; i < bufferSize; i++ )
//            poolOfClauses.push_back( new Clause( reserve ) );       
//    }
//    
//    Clause* back = poolOfClauses.back();
//    poolOfClauses.pop_back();
//    return back;
    return new Clause( reserve );
}

void
Solver::releaseClause(
    Clause* clause )
{
//    clause->free();    
//    poolOfClauses.push_back( clause );
    delete clause;
}

void
Solver::addPostPropagator(
    PostPropagator* postPropagator )
{
    if( !postPropagator->hasBeenAdded() )
    {
        postPropagators.push_back( postPropagator );
        postPropagator->onAdding();
    }
}

void
Solver::resetPostPropagators()
{
    while( !postPropagators.empty() )
    {
        PostPropagator* postPropagator = postPropagators.back();
        postPropagators.pop_back();
        postPropagator->onRemoving();
        postPropagator->reset();
    }
}

void
Solver::propagate(
    Var variable )
{
    assert( checkStatusBeforePropagation( variable ) );
    trace_msg( solving, 1, "Propagating " << ( isTrue( variable ) ? Literal( variable, POSITIVE ) : Literal( variable, NEGATIVE ) ) << " at level " << currentDecisionLevel );
    
    shortPropagation( variable );
    if( conflictDetected() )    
        return;    
    unitPropagation( variable );
}

void
Solver::propagateWithPropagators(
    Var variable )
{
    trace_msg( solving, 1, "Propagate with propagators" );
    propagate( variable );
    if( conflictDetected() )
        return;
    propagation( variable );
    if( conflictDetected() )
        return;
    postPropagation( variable );
    assert( !conflictDetected() );
}

uint64_t
Solver::computeCostOfModel(
    unsigned int level )
{
    uint64_t cost = getPrecomputedCost( level );
    for( unsigned int i = 0; i < numberOfOptimizationLiterals( level ); i++ )
    {        
        assert( getOptimizationLiteral( level, i ).lit != Literal::null );        
        //This is the first aux
        if( getOptimizationLiteral( level, i ).isAux() )
            continue;
        if( isTrue( getOptimizationLiteral( level, i ).lit ) )
            cost += getOptimizationLiteral( level, i ).weight;        
    }
    return cost;
}

void
Solver::computeCostOfModel(
    Vector< uint64_t >& costs )
{
    for( unsigned int i = 0; i < optimizationLiterals.size(); i++ )
        costs.push_back( computeCostOfModel( i ) );
}

//uint64_t
//Solver::computeCostOfModel()
//{
//    uint64_t cost = 0;
//    for( unsigned int j = 0; j < optimizationLiterals.size(); j++ )
//        cost += computeCostOfModel( j );    
//    return cost;
//}

void
Solver::setLevels(
    unsigned int n )
{
    for( unsigned int i = 0; i < n; i++ )
    {
        optimizationLiterals.push_back( vector< OptimizationLiteralData* >() );
        precomputedCosts.push_back( 0 );
        weighted_.push_back( false );
    }
    assert( optimizationLiterals.size() == precomputedCosts.size() );
}

void
Solver::addOptimizationLiteral(
    Literal lit,
    uint64_t weight,
    unsigned int level,
    bool aux )
{
    OptimizationLiteralData* optPointer = new OptimizationLiteralData();
    OptimizationLiteralData& opt = *optPointer;
    opt.lit = lit;
    opt.weight = weight;
    opt.level = level;
    opt.removed = 0;
    opt.aux = aux ? 1 : 0;
    assert_msg( level < optimizationLiterals.size(), level << " >= " << optimizationLiterals.size() );
    assert_msg( level < precomputedCosts.size(), level << " >= " << precomputedCosts.size() );
    assert_msg( level < weighted_.size(), level << " >= " << weighted_.size() );
    optimizationLiterals[ level ].push_back( optPointer );
    if( weight > 1 )
        setWeighted( level );
    getDataStructure( lit ).setOptLit( true );    
}

//uint64_t
//Solver::getCostOfLevel(
//    unsigned int levelIndex,
//    uint64_t totalCost ) const
//{    
//    assert_msg( levelIndex + 1 < maxCostOfLevelOfOptimizationRules.size(), "Index is " << levelIndex << " - Elements " << totalCost );
//    return ( totalCost % maxCostOfLevelOfOptimizationRules[ levelIndex + 1 ] ) / maxCostOfLevelOfOptimizationRules[ levelIndex ];
//}

void
Solver::addPreferredChoicesFromOptimizationLiterals(
    unsigned int level )
{
    assert( currentDecisionLevel == 0 );
    for( unsigned int i = 0; i < numberOfOptimizationLiterals( level ); i++ )
    {
        if( isUndefined( getOptimizationLiteral( level, i ).lit ) )
            minisatHeuristic->addPreferredChoice( getOptimizationLiteral( level, i ).lit.getOppositeLiteral() );
    }
}

void
Solver::clearOccurrences(
    Var v )
{
    if( variables.hasBeenEliminatedByDistribution( v ) )
        return;
    
    Literal lit( v, POSITIVE );
    getDataStructure( lit ).variableAllOccurrences.clearAndDelete();
    getDataStructure( lit.getOppositeLiteral() ).variableAllOccurrences.clearAndDelete();    
}

//void
//Solver::attachFirstWatch()
//{
//    assert( "Unary clause must be removed." && literals.size() > 1 );
//    literals[ 0 ].addWatchedClause( this );
//}
//
//void
//Solver::attachSecondWatch()
//{
//    assert( "Unary clause must be removed." && literals.size() > 1 );
//    literals[ 1 ].addWatchedClause( this );
//}

void
Solver::attachClause(
    Clause& clause )
{
    assert_msg( clause.size() > 1, "Clause " << clause << " is unary and it must be removed" );
    addWatchedClause( clause[ 0 ], &clause );
    addWatchedClause( clause[ 1 ], &clause );    
}

void
Solver::attachSecondWatch(
    Clause& clause )
{
    assert_msg( clause.size() > 1, "Clause " << clause << " is unary and it must be removed" );
    addWatchedClause( clause[ 1 ], &clause );    
}

void
Solver::attachClauseToAllLiterals(
    Clause& clause )
{
    unsigned int size = clause.size();
    for( unsigned int i = 0; i < size; i++ )
    {
        addClause( clause[ i ], &clause );
    }
}
//
//void
//Clause::attachClause( 
//    unsigned int first,
//    unsigned int second )
//{
//    assert( "First watch is out of range." && first < literals.size() );
//    assert( "Second watch is out of range." && second < literals.size() );
//    assert( "First watch and second watch point to the same literal." && first != second );   
//    
//    #ifndef NDEBUG
//    Literal tmp1 = literals[ first ];
//    Literal tmp2 = literals[ second ];
//    #endif
//
//    swapLiterals( 0, first );
//    second == 0 ? swapLiterals( 1, first ) : swapLiterals( 1, second );    
//
//    assert( literals[ 0 ] == tmp1 );
//    assert( literals[ 1 ] == tmp2 );
//    
//    attachFirstWatch();
//    attachSecondWatch();
//}

void
Solver::detachClause(
    Clause& clause )
{
    findAndEraseWatchedClause( clause[ 0 ], &clause );
    findAndEraseWatchedClause( clause[ 1 ], &clause );
}

void
Solver::detachClauseFromAllLiterals(
    Clause& clause,
    Literal literal )
{
    for( unsigned int i = 0; i < clause.size(); ++i )
    {
        if( clause[ i ] != literal )
            findAndEraseClause( clause[ i ], &clause );
    }    
}

void
Solver::detachClauseFromAllLiterals(
    Clause& clause )
{
    for( unsigned int i = 0; i < clause.size(); ++i )
        findAndEraseClause( clause[ i ], &clause );    
}

void
Solver::attachAggregate(
    Aggregate& aggregate )
{    
    assert( aggregate.size() > 0 );
    Literal aggregateLiteral = aggregate[ 1 ].getOppositeLiteral();
    addPropagator( aggregateLiteral, &aggregate, -1 );
    addPropagator( aggregateLiteral.getOppositeLiteral(), &aggregate, 1 );    
    for( unsigned int j = 2; j <= aggregate.size(); j++ )
    {
        Literal lit = aggregate[ j ];
        if( !isTrue( aggregateLiteral ) )
            addPropagator( lit.getOppositeLiteral(), &aggregate, -j );
        if( !isFalse( aggregateLiteral ) )
            addPropagator( lit, &aggregate, j );
    }
}

bool
Solver::isSatisfied(
    const Clause& clause ) const
{
    assert( clause.size() > 0 );
    
    unsigned i = 0;
    if( clause.hasBeenDeleted() )
        i = 1;
    
    for( ; i < clause.size(); ++i )
        if( isTrue( clause[ i ] ) )
            return true;
    return false;
}

bool
Solver::allUndefined(
    const Clause& clause ) const
{
    for( unsigned i = 0; i < clause.size(); ++i )
        if( !isUndefined( clause[ i ] ) )
            return false;
    return true;
}

bool
Solver::isLocked(
    const Clause& clause ) const
{
    assert_msg( clause.size() > 1, "Clause " << clause << " is unary and it must be removed" );

    //We assume that the literal inferred is always in the first position.
    return isImplicant( clause[ 0 ].getVariable(), &clause );
}

void 
Solver::markAllClauses(
    Var variable )
{
    Literal pos = Literal( variable, POSITIVE );
    Literal neg = Literal( variable, NEGATIVE );
    
    Vector< Clause* >& posAllOccurrences = getDataStructure( pos ).variableAllOccurrences;
    
    for( unsigned i = 0; i < posAllOccurrences.size(); ++i )
    {
        Clause* clause = posAllOccurrences[ i ];
        detachClauseFromAllLiterals( *clause, pos );        
        markClauseForDeletion( clause );
    }
    posAllOccurrences.clearAndDelete();

    Vector< Clause* >& negAllOccurrences = getDataStructure( neg ).variableAllOccurrences;
    for( unsigned i = 0; i < negAllOccurrences.size(); ++i )
    {
        Clause* clause = negAllOccurrences[ i ];
        detachClauseFromAllLiterals( *clause, neg );
        markClauseForDeletion( clause );
    }
    negAllOccurrences.clearAndDelete();
}

void
Solver::removeAllClauses(
    Var v )
{
    Literal pos( v, POSITIVE );
    Literal neg( v, NEGATIVE );    
    
    Vector< Clause* >& posAllOccurrences = getDataStructure( pos ).variableAllOccurrences;
    for( unsigned i = 0; i < posAllOccurrences.size(); ++i )
    {
        Clause* clause = posAllOccurrences[ i ];
        removeClauseNoDeletion( clause );
        removeLiteralAndMarkClause( *clause, pos );
    }
    posAllOccurrences.clearAndDelete();

    Vector< Clause* >& negAllOccurrences = getDataStructure( neg ).variableAllOccurrences;
    for( unsigned i = 0; i < negAllOccurrences.size(); ++i )
    {
        Clause* clause = negAllOccurrences[ i ];
        removeClauseNoDeletion( clause );
        removeLiteralAndMarkClause( *clause, neg );
    }
    negAllOccurrences.clearAndDelete();
}

void
Solver::removeLiteralAndMarkClause(
    Clause& clause,
    Literal literal )
{
    for( unsigned int i = 0; i < clause.size(); ++i )
    {
        if( clause[ i ] != literal )
            findAndEraseClause( clause[ i ], &clause );
    }
    
    assert( clause.size() != 0 );
    clause.markAsDeleted();
}

Literal
Solver::getLiteralWithMinOccurrences(
    const Clause& clause ) const
{
    assert( clause.size() > 1 );
    Literal minLiteral = clause[ 0 ];
//    assert( numberOfOccurrences( minLiteral ) > 0 );

    unsigned int i = 1;
    do
    {
//        assert( numberOfOccurrences( clause[ i ] ) > 0 );
        if( numberOfOccurrences( clause[ i ] ) < numberOfOccurrences( minLiteral ) )
        {
            minLiteral = clause[ i ];
        }
    } while( ++i < clause.size() );    
        
    return minLiteral;
}

Var
Solver::getVariableWithMinOccurrences(
    const Clause& clause ) const
{
    assert( clause.size() > 1 );
    assert( !clause.hasBeenDeleted() );
    Var minVariable = clause[ 0 ].getVariable();
    assert_msg( numberOfOccurrences( minVariable ) > 0, "Variable " << minVariable << " does not know to occur in " << clause );

    unsigned int i = 1;
    do
    {
        assert_msg( numberOfOccurrences( clause[ i ].getVariable() ) > 0, "Variable " << clause[ i ].getVariable() << " does not know to occur in " << clause );
        if( numberOfOccurrences( clause[ i ].getVariable() ) < numberOfOccurrences( minVariable ) )
        {
            minVariable = clause[ i ].getVariable();
        }
    } while( ++i < clause.size() );    
        
    return minVariable;
}

bool
Solver::removeSatisfiedLiterals(
    Clause& clause )
{
    if( isTrue( clause[ 0 ] ) )
    {        
        if( isLocked( clause ) )
            setImplicant( clause[ 0 ].getVariable(), NULL );
        return true;        
    }
    
    assert_msg( !isFalse( clause[ 0 ] ), "Literal " << clause[ 0 ] <<  " in clause " << clause << " is false" );
    assert_msg( !isFalse( clause[ 1 ] ), "Literal " << clause[ 1 ] <<  " in clause " << clause << " is false" );

    if( isTrue( clause[ 1 ] ) )
        return true;        
    
    for( unsigned int i = 2; i < clause.size(); )
    {
        if( isTrue( clause[ i ] ) )
            return true;

        if( isFalse( clause[ i ] ) )
        {
            clause[ i ] = clause[ clause.size() - 1 ];
            clause.removeLastLiteralNoWatches();            
        }
        else
        {
            i++;
        }        
    }
        
    return false;
}

bool
Solver::onLiteralFalse(
    Clause& clause,
    Literal literal )
{
    assert_msg( isFalse( literal ), "Literal " << literal << " is not false");
    assert_msg( clause.size() > 1, "Clause " << clause << " is unary and it must be removed" );

    if( literal == clause[ 0 ] )
    {
        //The watch to update should be always in position 1.
        clause[ 0 ] = clause[ 1 ];
        clause[ 1 ] = literal;
    }

    assert_msg( literal == clause[ 1 ], "Literal " << literal << " should be in position 1, but literal " << clause[ 1 ] << "is in position 1" );
    //if the clause is already satisfied do nothing.
    if( isTrue( clause[ 0 ] ) )
        return false;
    
    for( unsigned i = 2; i < clause.size(); ++i )
    {
        if( !isFalse( clause[ i ] ) )
        {
            //Swap the two literals
            clause.swapLiteralsNoWatches( 1, i );            

            //Attach the watch in the new position
            attachSecondWatch( clause );
            return false;
        }
    }

    assert( "The other watched literal cannot be true." && !isTrue( clause[ 0 ] ) );    
    //Propagate clause[ 0 ];
    return true;
}

void
Solver::learnedClauseUsedForConflict(
    Clause* clausePointer )
{
    updateActivity( clausePointer );
    if( glucoseHeuristic_ )
    {
        Clause& clause = *clausePointer;
        if( clause.lbd() > 2 )
        {
            unsigned lbd = computeLBD( clause );
            if( lbd + 1 < clause.lbd() )
            {
                if( clause.lbd() <= glucoseData.lbLBDFrozenClause )
                {
                    clause.setCanBeDeleted( false );
                }
                clause.setLbd( lbd );
            }
        }
    }
}

unsigned int
Solver::computeLBD(
    const Clause& clause )
{
    assert( glucoseHeuristic_ );
    unsigned int lbd = 0;
    glucoseData.MYFLAG++;
    for( unsigned int i = 0; i < clause.size(); i++ )
    {
        if( isAssumption( clause[ i ].getVariable() ) )
            continue;
        unsigned int level = getDecisionLevel( clause[ i ] );        
        if( glucoseData.permDiff[ level ] != glucoseData.MYFLAG )
        {
            glucoseData.permDiff[ level ] = glucoseData.MYFLAG;
            lbd++;
        }
    }
	return lbd;
}

void
Solver::deleteClausesIfNecessary()
{
    if( glucoseHeuristic_ )
    {
        if( numberOfLearnedClauses() >= glucoseData.currRestart * glucoseData.nbclausesBeforeReduce )
        {
            assert( numberOfLearnedClauses() > 0 );
            glucoseData.currRestart = ( conflicts / glucoseData.nbclausesBeforeReduce ) + 1;
            deleteClauses();
            glucoseData.nbclausesBeforeReduce += glucoseData.incReduceDB;
        }
    }
    else
    {
        if( ( int ) ( numberOfLearnedClauses() - numberOfAssignedLiterals() ) >= deletionCounters.maxLearned )
            deleteClauses();
    }
}

bool
Solver::restartIfNecessary()
{
    if( currentDecisionLevel == 0 )
        return true;
    bool hasToRestart = glucoseHeuristic_ ? ( glucoseData.lbdQueue.isValid() && ( ( glucoseData.lbdQueue.getAvg() * glucoseData.K ) > ( glucoseData.sumLBD / conflictsRestarts ) ) ) : restart->hasToRestart();
    if( hasToRestart )
    {
        statistics( this, onRestart() );    
        glucoseData.lbdQueue.fastClear();
        if( !doRestart() )
            return false;
        simplifyOnRestart();
    }
    
    return true;
}

bool
Solver::minimisationWithBinaryResolution(
    Clause& learnedClause,
    unsigned int lbd )
{
    if( lbd <= glucoseData.lbLBDMinimizingClause )
    {
        Literal p = learnedClause[ 0 ];//.getOppositeLiteral();
        glucoseData.MYFLAG++;

        for( unsigned int i = 1; i < learnedClause.size(); i++ )
            glucoseData.permDiff[ learnedClause[ i ].getVariable() ] = glucoseData.MYFLAG;

        Vector< Literal >& wbin = getDataStructure( p ).variableBinaryClauses;        
        int nb = 0;
        for( unsigned int k = 0; k < wbin.size(); k++ )
        {
            Literal imp = wbin[ k ];
            if( glucoseData.permDiff[ imp.getVariable() ] == glucoseData.MYFLAG && isTrue( imp ) )
            {
                nb++;
                glucoseData.permDiff[ imp.getVariable() ] = glucoseData.MYFLAG - 1;
            }
        }
        
        int l = learnedClause.size() - 1;
        if( nb > 0 )
        {
            for( unsigned int i = 1; i < learnedClause.size() - nb; i++ )
            {
                if( glucoseData.permDiff[ learnedClause[ i ].getVariable() ] != glucoseData.MYFLAG )
                {
                    Literal p = learnedClause[ l ];
                    learnedClause[ l ] = learnedClause[ i ];
                    learnedClause[ i ] = p;
                    l--;
                    i--;
                }
            }
            learnedClause.shrink( learnedClause.size() - nb );
            return true;
        }
    }
    
    return false;
}

void
Solver::computeStrongConnectedComponents()
{
    dependencyGraph->computeStrongConnectedComponents();
    if( !dependencyGraph->tight() )
        createCyclicComponents();

    delete dependencyGraph;
    dependencyGraph = NULL;    
}

bool
Solver::modelIsValidUnderAssumptions(
    vector< Literal >& assumptions )
{
    trace_msg( solving, 1, "Check assumptions" );
    for( unsigned int i = 0; i < assumptions.size(); i++ )
    {
        trace_msg( solving, 2, "Checking " << assumptions[ i ] );
        if( isFalse( assumptions[ i ] ) )
        {
            conflictLiteral = assumptions[ i ];
            trace_msg( solving, 3, "Assumption " << conflictLiteral << " not satisfied" );            
            if( computeUnsatCores_ )
            {
                assert( unsatCore == NULL );
                computeUnsatCore();                
            }
            return false;
        }
    }
    trace_msg( solving, 2, "Assumptions satisfied" );
    return true;
}

OptimizationLiteralData&
Solver::getOptimizationLiteral(
    unsigned int level,
    unsigned int pos )
{
    assert( level < optimizationLiterals.size() );    
    assert( pos < optimizationLiterals[ level ].size() );
    return *optimizationLiterals[ level ][ pos ];
}

void
Solver::computeUnsatCore()
{
    assert( conflictLiteral != Literal::null );
//    delete unsatCore;
    unsatCore = learning.analyzeFinal( conflictLiteral );
}

void
Solver::minimizeUnsatCore(
    vector< Literal >& assumptions )
{
    unsigned int originalMaxNumberOfChoices = maxNumberOfChoices;
    unsigned int originalMaxNumberOfRestarts = maxNumberOfRestarts;
    
    setMaxNumberOfChoices( UINT_MAX );
    setMaxNumberOfRestarts( UINT_MAX );
    begin:;

    assert( unsatCore != NULL );
    if( unsatCore->size() <= 2 )
    {
        setMaxNumberOfChoices( originalMaxNumberOfChoices );
        setMaxNumberOfRestarts( originalMaxNumberOfRestarts );
        return;
    }
    
    vector< Literal > tmp;    
    clearAfterSolveUnderAssumptions( assumptions );
    clearConflictStatus();    
//    for( unsigned int i = 0; i < assumptions.size(); i++ )
//    {
//        Literal lit = assumptions[ i ];
//        if( getDataStructure( lit ).isOptLit() || getDataStructure( lit.getOppositeLiteral() ).isOptLit() )
//            continue;
//        
//        tmp.push_back( lit );
//        setAssumptionAND( lit, true );
//    }
    
    assumptions.swap( tmp );
    
    for( unsigned int i = 0; i < unsatCore->size(); i++ )
    {
        Literal lit = unsatCore->getAt( i );        
        if( !getDataStructure( lit ).isOptLit() /*&& !getDataStructure( lit.getOppositeLiteral() ).isOptLit()*/ )
            continue;
                
        Literal toAdd = lit.getOppositeLiteral();
//        if( getDataStructure( lit ).isOptLit() )            
//            toAdd = lit.getOppositeLiteral();
//        else if( getDataStructure( lit.getOppositeLiteral() ).isOptLit() )
//            toAdd = lit;
//        else
//            continue;
        assumptions.push_back( toAdd );
        setAssumption( toAdd, true );
    }
    numberOfAssumptions = assumptions.size();
    
    unrollToZero();
    unsigned int oldSize = unsatCore->size();
    delete unsatCore;
    unsatCore = NULL;

    #ifndef NDEBUG
    unsigned int result = 
    #endif
    ( !hasPropagators() ) ? solveWithoutPropagators( assumptions ) : solvePropagators( assumptions );
    assert( result == INCOHERENT );

    if( unsatCore == NULL )
        unsatCore = new Clause();
    
    assert( unsatCore->size() <= oldSize );
    if( unsatCore->size() < oldSize )
        goto begin;
    
    assert( unsatCore->size() == oldSize );
    setMaxNumberOfChoices( originalMaxNumberOfChoices );
    setMaxNumberOfRestarts( originalMaxNumberOfRestarts );
}

//Aggregate*
//Solver::createAggregate(
//    const vector< Literal >& literals,
//    const vector< unsigned int >& weights )
//{
//    assert( literals.size() == weights.size() );    
//    Aggregate* aggregate = new Aggregate();
//    for( unsigned int i = 0; i < literals.size(); i++ )
//        aggregate->addLiteral( literals[ i ], weights[ i ] );    
//    
//    return aggregate;
//}

void
Solver::addInPropagatorsForUnroll(
    Propagator* prop )
{
    assert( prop != NULL );
    unsigned int dl = getCurrentDecisionLevel();
    while( fromLevelToPropagators.size() <= dl )
        fromLevelToPropagators.push_back( propagatorsForUnroll.size() );
        
    if( !prop->isInVectorOfUnroll( dl ) )
    {
        propagatorsForUnroll.push_back( prop );
        prop->setInVectorOfUnroll( dl );
    }
}

bool
Solver::propagateFixpoint()
{    
    assert( !conflictDetected() );
    while( hasNextVariableToPropagate() )
    {
        nextValueOfPropagation--;
        Var variableToPropagate = getNextVariableToPropagate();
        propagateWithPropagators( variableToPropagate );

        if( conflictDetected() )
            return false;
    }
    return true;
}

uint64_t
Solver::simplifyOptimizationLiterals(
    unsigned int level )
{
    assert( getPrecomputedCost( level ) == 0 );
    vector< OptimizationLiteralData* >& optLiterals = optimizationLiterals[ level ];
    unsigned int j = 0;
    for( unsigned int i = 0; i < optLiterals.size(); i++ )
    {
        optLiterals[ j ] = optLiterals[ i ];
        if( isTrue( optLiterals[ j ]->lit ) )
            precomputedCosts[ level ] += optLiterals[ j ]->weight;
        else
            ++j;
    }
    optLiterals.resize( j );
    return precomputedCosts[ level ];
}

//void
//Solver::simplifyOptimizationLiterals()
//{
//    assert( precomputedCost == 0 );
//    for( unsigned int l = 0; l < optimizationLiterals.size(); l++ )
//        precomputedCost += simplifyOptimizationLiterals( l );    
//}

#endif
