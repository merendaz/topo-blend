#include "DynamicGraph.h"

DynamicGraph::DynamicGraph(Structure::Graph *fromGraph)
{
    this->mGraph = fromGraph;
	this->uniqueID = 0;

	if(mGraph == NULL) return;
	
    // Add nodes
    foreach(Structure::Node * n, mGraph->nodes)
        addNode( SingleProperty("original", n->id) );

    // Add edges
    foreach(Structure::Link e, mGraph->edges)
        addEdge( nodeIndex("original", e.n1->id), nodeIndex("original", e.n2->id) );
}

DynamicGraph DynamicGraph::clone()
{
	DynamicGraph g;
	g.mGraph	= this->mGraph;
	g.nodes		= this->nodes;
	g.edges		= this->edges;
	g.adjacency = this->adjacency;
	g.uniqueID	= this->uniqueID;
	return g;
}

int DynamicGraph::addNode(Properties properties, int index)
{
    if(index < 0) index = nodes.size();
    nodes[index] = SimpleNode(index);
    nodes[index].property = properties;

	// Always increasing global identifier
	uniqueID++;

	return index;
}

void DynamicGraph::addEdge(int fromNode, int toNode)
{
    if(fromNode == toNode) assert(0);

    SimpleEdge e(fromNode, toNode);

    edges[edges.size()] = e;

    adjacency[fromNode].insert(e);
    adjacency[toNode].insert(e);
}

int DynamicGraph::nodeIndex(QString property_name, QString property_value)
{
    foreach(SimpleNode n, nodes)
        if(n.property.contains(property_name) && n.property[property_name] == property_value)
            return n.idx;
    return -1;
}

QString DynamicGraph::nodeType( int index )
{
	return mGraph->getNode(nodes[index].str("original"))->type();
}

void DynamicGraph::removeNode( int idx )
{
	// Remove edges
	foreach(int adj, adjNodes(idx))
		removeEdge(idx, adj);

	// Remove node and adjacency list
	nodes.remove(idx);
	adjacency.remove(idx);
}

void DynamicGraph::removeEdge(int fromNode, int toNode)
{
	if(fromNode == toNode) assert(0);

	SimpleEdge e(fromNode, toNode);
	
	QMapIterator<int, SimpleEdge> i(edges);
	while (i.hasNext()) {
		i.next();
		if(i.value() == e){
			edges.remove(i.key());
			break;
		}
	}

	adjacency[fromNode].remove(e);
	adjacency[toNode].remove(e);
}

QSet<int> DynamicGraph::adjNodes(int index)
{
	QSet<int> adj;

	foreach(SimpleEdge e, adjacency[index])
		adj.insert( e.n[0] == index ? e.n[1] : e.n[0] );

	return adj;
}

GraphState DynamicGraph::State()
{
	GraphState state;

	state.numSheets = numSheets();
	state.numCurves = numCurves();

	state.numSheetEdges = numEdges(SAME_SHEET);
	state.numCurveEdges = numEdges(SAME_CURVE);
	state.numMixedEdges = numEdges(CURVE_SHEET);

	return state;
}

void DynamicGraph::printState()
{
	GraphState state = this->State();
	state.print();
}

int DynamicGraph::numEdges( EdgeType t )
{
	int count = 0;

	foreach(SimpleEdge e, edges){
		switch(t)
		{
		case ANY_EDGE	: count++; break;
		case CURVE_SHEET:if(nodeType(e.n[0]) != nodeType(e.n[1]))	count++; break;
		case SAME_CURVE	:if(nodeType(e.n[0]) == Structure::CURVE && nodeType(e.n[0]) == nodeType(e.n[1]))	count++; break;
		case SAME_SHEET	:if(nodeType(e.n[0]) == Structure::SHEET && nodeType(e.n[0]) == nodeType(e.n[1]))	count++; break;
		}
	}

	return count;
}

QVector<int> DynamicGraph::getSheets()
{
	QVector<int> sheets;
	foreach(SimpleNode n, nodes) 
		if(nodeType(n.idx) == Structure::SHEET)
			sheets.push_back(n.idx);
	return sheets;
}

int DynamicGraph::numSheets()
{
	return getSheets().size();
}

QVector<int> DynamicGraph::getCurves()
{
	QVector<int> curves;
	foreach(SimpleNode n, nodes) 
		if(nodeType(n.idx) == Structure::CURVE)
			curves.push_back(n.idx);
	return curves;
}

int DynamicGraph::numCurves()
{
	return getCurves().size();
}

int DynamicGraph::cloneNode( int index, bool cloneEdges )
{
	int newIndex = addNode( nodes[index].property, uniqueID );

	if(cloneEdges){
		QSet<int> adj = adjNodes( index );

		foreach(int ni, adj)
			addEdge(newIndex, ni);
	}

	return newIndex;
}

void DynamicGraph::printNodeInfo( int index )
{
	qDebug() << "\nNode:";
	qDebug() << " idx  : " << nodes[index].idx;
	qDebug() << " type : " << nodeType(index);

	// Adjacent info
	QStringList adjList; 
	foreach(int ni, adjNodes(index)) adjList << QString::number(ni);
	qDebug() << " adj  : " << adjList.join(", ");
}

GraphState DynamicGraph::difference( GraphState & other )
{
	GraphState diff;
	GraphState my = State();

	diff.numSheets = my.numSheets - other.numSheets;
	diff.numCurves = my.numCurves - other.numCurves;
	diff.numSheetEdges = my.numSheetEdges - other.numSheetEdges;
	diff.numCurveEdges = my.numCurveEdges - other.numCurveEdges;
	diff.numMixedEdges = my.numMixedEdges - other.numMixedEdges;

	return diff;
}

Structure::Graph * DynamicGraph::toStructureGraph(DynamicGraph & target)
{
	Structure::Graph * graph = new Structure::Graph();
	QMap<int,QString> nodeMap;

	double score = 0;
	QVector<QPairInt> corr = correspondence(target, score, true);
	QMap<int,int> corrMap;
	foreach(QPairInt p, corr) corrMap[p.first] = p.second;

	// Add nodes
	foreach(SimpleNode n, nodes)
	{
		int oldIdx = n.idx;
		int newIdx = corrMap[n.idx];

		QString oldId = nodes[oldIdx].str("original");
		QString nodeId = target.nodes[ newIdx ].str("original");

		if(nodeType(n.idx) == Structure::SHEET)
		{
			Structure::Sheet * s = (Structure::Sheet *) mGraph->getNode(n.str("original"));
			graph->addNode( new Structure::Sheet(s->surface, nodeId) );
			nodeMap[n.idx] = nodeId;
		}

		if(nodeType(n.idx) == Structure::CURVE)
		{
			Structure::Curve * s = (Structure::Curve *) mGraph->getNode(n.str("original"));
			graph->addNode( new Structure::Curve(s->curve, nodeId) );
			nodeMap[n.idx] = nodeId;
		}
	}

	// Add edges
	foreach(SimpleEdge e, target.edges)
	{
		QString id1 = target.nodes[e.n[0]].str("original");
		QString id2 = target.nodes[e.n[1]].str("original");

		Structure::Node *n1 = graph->getNode(id1), *n2 = graph->getNode(id2);

		graph->addEdge( n1, n2, Vec2d(0), Vec2d(0), graph->linkName(n1, n2) );

		// Copy edge coordinates
		Structure::Link *fromEdge = target.mGraph->getEdge(id1, id2);
		Structure::Link *toEdge = graph->getEdge(id1, id2);

		toEdge->setCoord(id1, fromEdge->getCoord(id1));
		toEdge->setCoord(id2, fromEdge->getCoord(id2));
	}

	return graph;
}

QVector<DynamicGraph> DynamicGraph::candidateNodes(DynamicGraph & targetGraph)
{
	QVector<DynamicGraph> candidate;
	GraphState futureState = targetGraph.State();

	// Compute graph difference
	GraphState diff = difference( futureState );

	// Should return no candidates when they are exactly the same
	if( diff.isZero() ) return candidate;

	QVector<int> activeNodes;
	bool isAdding = true;
	int discrepancy = 0;

	// Missing nodes: nodes have precedent
	if(diff.numSheets != 0 || diff.numCurves != 0)
	{
		// Sheets and curves: sheet have precedent
		if(diff.numSheets != 0)
		{
			activeNodes = getSheets();
			if(diff.numSheets > 0) isAdding = false;
			discrepancy = abs(diff.numSheets);
		}
		else if(diff.numCurves != 0)
		{
			activeNodes = getCurves();
			if(diff.numCurves > 0) isAdding = false;
			discrepancy = abs(diff.numCurves);
		}
	}

	if(activeNodes.size() < 1) return candidate;

	// Try all sets of new edges of size
	std::vector<int> nodesSet = activeNodes.toStdVector();
	std::vector<int>::iterator begin = nodesSet.begin(), end = nodesSet.end();
	do {
		std::vector<int> currentSet (begin, begin + discrepancy);

		// Add operation as a candidate graph
		DynamicGraph g = clone();

		foreach(int ci, currentSet)
		{
			if(isAdding)	
				g.cloneNode(ci);
			else
				g.removeNode(ci);
		}

		candidate.push_back(g);

	} while (next_combination(begin, begin + discrepancy, end));

	return candidate;
}

QVector<DynamicGraph> DynamicGraph::candidateEdges(DynamicGraph & targetGraph)
{
	QVector<DynamicGraph> candidate;
	GraphState futureState = targetGraph.State();

	// Compute graph difference
	GraphState diff = difference( futureState );

	// Should return no candidates when they are exactly the same
	if( diff.isZero() ) return candidate;

	QVector<int> groupA, groupB;
	QSet<SimpleEdge> exploredEdges;
	bool isAdding = true;

	int discrepancy = 0;

	// Add existing edges as explored
	foreach(SimpleEdge e, edges)
		exploredEdges.insert(e);

	if(diff.numMixedEdges != 0)
	{
		// Curve-Sheet edges
		if(diff.numMixedEdges > 0) isAdding = false;
		groupA = getSheets();
		groupB = getCurves();
		discrepancy = abs(diff.numMixedEdges);
	}
	else if(diff.numSheetEdges != 0)
	{
		// Sheet-Sheet edges
		if(diff.numSheetEdges > 0) isAdding = false;
		groupA = getSheets();
		groupB = groupA;
		discrepancy = abs(diff.numSheetEdges);
	}
	else if(diff.numCurveEdges != 0)
	{
		// Curve-Curve edges
		if(diff.numCurveEdges > 0) isAdding = false;
		groupA = getCurves();
		groupB = groupA;
		discrepancy = abs(diff.numCurveEdges);
	}

	// Find all possible new edges
	QMap<int, SimpleEdge> uniqueEdges;
	foreach(int a, groupA)
	{
		foreach(int b, groupB)
		{
			SimpleEdge e(a, b);

			// Uniqueness check:
			if(a == b || exploredEdges.contains(e)) continue;
			exploredEdges.insert(e);

			uniqueEdges[uniqueEdges.size()] = e;
		}
	}

	// Try all sets of new edges of size
	std::vector<int> uniqueEdgesIdx;
	foreach(int key, uniqueEdges.keys()) uniqueEdgesIdx.push_back(key);

	std::vector<int>::iterator begin = uniqueEdgesIdx.begin(), end = uniqueEdgesIdx.end();
	do {
		std::vector<int> currentSet (begin, begin + discrepancy);

		// Add operation as a candidate graph
		DynamicGraph g = clone();

		foreach(int key, currentSet)
		{
			SimpleEdge e = uniqueEdges[key];

			if(isAdding)
				g.addEdge(e.n[0], e.n[1]);
			else
				g.removeEdge(e.n[0], e.n[1]);
		}

		candidate.push_back(g);

	} while (next_combination(begin, begin + discrepancy, end));

	return candidate;
}

int DynamicGraph::valence( int nodeIndex )
{
	return adjacency[nodeIndex].size();
}

QVector< QPairInt > DynamicGraph::valences(bool isPrint)
{
	std::vector<int> vals;
	std::vector<unsigned int> indx;

	foreach(int ni, nodes.keys())
	{
		vals.push_back( adjacency[ni].size() );
		indx.push_back( ni );
	}

	// Sort by valence
	paired_sort(indx, vals, true);

	// DEBUG:
	if(isPrint){
		QStringList vstr;
		foreach(int v, vals) vstr << QString::number(v);
		qDebug() << vstr;
	}

	QVector< QPairInt > valPair;

	for(int i = 0; i < (int) vals.size(); i++)
		valPair.push_back( qMakePair(vals[i], (int)indx[i]) );

	return valPair;
}

bool DynamicGraph::sameValences( DynamicGraph & other )
{
	QVector< QPairInt > myValence = valences();
	QVector< QPairInt > otherValence = other.valences();

	for(int i = 0; i < (int) myValence.size(); i++){
		if(otherValence[i].first != myValence[i].first)
			return false;
	}

	return true;
}

QVector< QPairInt > DynamicGraph::correspondence( DynamicGraph & other, double & score, bool isPrint )
{
	QVector< QPairInt > corr;

	QVector< QPairInt > vme = valences();
	QVector< QPairInt > vother = other.valences();

	// Build the set for the other graph
	QMap< int, QList<int> > vset;

	foreach(QPairInt p, vother)
	{
		vset[ p.first ].push_back( p.second );
	}

	if(isPrint) qDebug() << "Correspondence map:";

	// Test correspondence score and assign to smallest
	foreach(QPairInt p, vme)
	{
		int valence = p.first;
		int nodeID = p.second;

		QMap<double, int> scoreBoard;

		QString n1_id = nodes[nodeID].str("original");
		Structure::Node * n1 = this->mGraph->getNode( n1_id );
		Vector3 center1(0); n1->get(Vector3(0.5), center1);

		// Test against possible corresponding nodes
		foreach( int curID, vset[ valence ] )
		{
			QString n2_id = other.nodes[curID].str("original");
			Structure::Node * n2 = other.mGraph->getNode( n2_id );
			Vector3 center2(0); n2->get(Vector3(0.5), center2);

			double currScore = (center1 - center2).norm();

			scoreBoard[ currScore ] = curID;
		}

		// Add minimum score to total
		double minScore = scoreBoard.keys().first();
		score += minScore;

		int otherID = scoreBoard[ minScore ];
		vset[ valence ].removeAll( otherID );

		// Pair current node with minimum one
		corr.push_back( qMakePair(nodeID, otherID) );

		if(isPrint)
			qDebug() << nodes[nodeID].str("original") << " <--> " << other.nodes[otherID].str("original");
	}

	if(isPrint) qDebug() << "===\n";

	return corr;
}

void DynamicGraph::flagNodes( QString propertyName, QVariant value )
{
	for(int i = 0; i < numNodes(); i++)
		nodes[i].property[propertyName] = value;
}

QVector<QVariant> DynamicGraph::flags( QString propertyName )
{
	QVector<QVariant> f;
	for(int i = 0; i < numNodes(); i++)
		f.push_back(nodes[i].property[propertyName]);
	return f;
}

SimpleNode * DynamicGraph::getNode( QString originalID )
{
	for(int i = 0; i < numNodes(); i++)
		if(nodes[i].str("original") == originalID)
			return &nodes[i];
	return NULL;
}

/*
void DynamicGraph::correspondTo( DynamicGraph & other )
{
	double score = 0;
	QVector< QPairInt > corr = correspondence(other, score);

	foreach( QPairInt p, corr )
	{
		int from = p.first;
		int to = p.second;

		if(from == to) continue;

		// Swap
		QString from_original = nodes[from].property["original"];
		QString to_original = nodes[to].property["original"];

		nodes[from].property["original"] = to_original;
		nodes[to].property["original"] = from_original;
	}
}*/
