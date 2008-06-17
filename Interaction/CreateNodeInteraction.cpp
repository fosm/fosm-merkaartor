#include "CreateNodeInteraction.h"

#include "MainWindow.h"
#include "PropertiesDock.h"
#include "Command/DocumentCommands.h"
#include "Command/RoadCommands.h"
#include "Map/Projection.h"
#include "Map/TrackPoint.h"
#include "Utils/LineF.h"

#include <vector>

CreateNodeInteraction::CreateNodeInteraction(MapView* aView)
: RoadSnapInteraction(aView)
{
}

CreateNodeInteraction::~CreateNodeInteraction(void)
{
}

void CreateNodeInteraction::snapMouseReleaseEvent(QMouseEvent * ev, Road* aRoad)
{
	if (ev->button() == Qt::LeftButton)
	{
		Coord P(projection().inverse(ev->pos()));
		if (aRoad)
		{
			main()->properties()->setSelection(0);
			CommandList* theList  = new CommandList(MainWindow::tr("Create node in Road: %1").arg(aRoad->id()), aRoad);
			unsigned int SnapIdx = findSnapPointIndex(aRoad, P);
			TrackPoint* N = new TrackPoint(P);
			N->setTag("created_by", QString("Merkaartor %1").arg(VERSION));
			theList->add(new AddFeatureCommand(main()->document()->getDirtyLayer(),N,true));
			theList->add(new RoadAddTrackPointCommand(aRoad,N,SnapIdx,main()->document()->getDirtyLayer()));
			document()->addHistory(theList);
			view()->invalidate();
		}
		else
		{
			TrackPoint* N = new TrackPoint(P);
			N->setTag("created_by", QString("Merkaartor %1").arg(VERSION));
			CommandList* theList  = new CommandList(MainWindow::tr("Create point %1").arg(N->id()), aRoad);
			theList->add(new AddFeatureCommand(main()->document()->getDirtyLayer(),N,true));
			document()->addHistory(theList);
			view()->invalidate();
		}
	}
}


QCursor CreateNodeInteraction::cursor() const
{
	return QCursor(Qt::CrossCursor);
}




