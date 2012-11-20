#pragma once
#include "DynamicGraphGlobal.h"
#include "StructureGraph.h"

class DynamicGraph
{
public:

    DynamicGraph(Structure::Graph * fromGraph = 0);
	DynamicGraph clone();

	// ADD
    int addNode(Properties properties = noProperties(), int index = -1);
	void addEdge(int fromNode, int toNode);

	int cloneNode(int index, bool cloneEdges = false);

	// REMOVE
	void removeNode( int idx );	
	void removeEdge(int fromNode, int toNode);

	// GET
    int nodeIndex(QString property_name, QString property_value);
	QSet<int> adjNodes(int index);
	QString nodeType(int index);
	SimpleNode * getNode( QString originalID );

	int numNodes(){ return nodes.size(); }
	int numEdges(EdgeType t);
	int numSheets();
	int numCurves();

	QVector<int> getSheets();
	QVector<int> getCurves();

	// State
	GraphState State();
	void printState();
	void printNodeInfo(int index);
	GraphState difference(GraphState & other);
	operator GraphState() { return State(); }

	// Flags
	void flagNodes(QString propertyName, QVariant value);
	QVector<QVariant> flags(QString propertyName);

	// Valence based operations
	int valence(int nodeIndex);
	QVector< QPairInt > DynamicGraph::valences(bool isPrint = false);
	bool sameValences( DynamicGraph & other );
	QVector< QPairInt > DynamicGraph::correspondence( DynamicGraph & other, double & score, bool isPrint = false );
	//void correspondTo( DynamicGraph & other );

	// Generate structure graph
	Structure::Graph * toStructureGraph(DynamicGraph & target);

	// Graph edit
	QVector<DynamicGraph> candidateNodes(DynamicGraph & targetGraph);
	QVector<DynamicGraph> candidateEdges(DynamicGraph & targetGraph);

public:

	// Properties
	Structure::Graph * mGraph;
	int uniqueID;
	Structure::Graph * graph(){ return mGraph; }

	QMap<int, SimpleNode> nodes;
	QMap<int, SimpleEdge> edges;
	QMap<int, QSet<SimpleEdge> > adjacency;
};
