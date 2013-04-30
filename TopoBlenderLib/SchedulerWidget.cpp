#include "Task.h"
#include "SchedulerWidget.h"
#include "ui_SchedulerWidget.h"

SchedulerWidget::SchedulerWidget(Scheduler * scheduler, QWidget *parent) : QWidget(parent), ui(new Ui::SchedulerWidget), s(scheduler)
{
    ui->setupUi(this);
    ui->timelineView->setScene(scheduler);
	ui->timelineView->updateSceneRect(scheduler->sceneRect());

	// Add nodes to list
	foreach(Task * t, scheduler->tasks)
	{
		ui->nodesList->addItem(t->property["nodeID"].toString());
	}

	scheduler->connect( ui->blendButton, SIGNAL(clicked()), SLOT(doBlend()) );

	connect( scheduler, SIGNAL(progressChanged(int)), ui->progressBar, SLOT(setValue(int)) );

	this->connect( scheduler, SIGNAL(progressStarted()), SLOT(progressStarted()) );
	this->connect( scheduler, SIGNAL(progressDone()), SLOT(progressDone()) );

	scheduler->connect( ui->stopButton, SIGNAL(clicked()), SLOT(stopExecution()) );
	scheduler->connect( ui->sameTimeButton, SIGNAL(clicked()), SLOT(startAllSameTime()) );

	scheduler->connect( ui->renderCurrentButton, SIGNAL(clicked()), SLOT(doRenderCurrent()) );
	scheduler->connect( ui->renderAllButton, SIGNAL(clicked()), SLOT(doRenderAll()) );

	scheduler->connect( ui->draftRenderButton, SIGNAL(clicked()), SLOT(doDraftRender()));

	// Render frames count
	this->connect( ui->renderCount, SIGNAL(valueChanged(int)), SLOT(changeRenderCount(int)));
	scheduler->property["renderCount"] = ui->renderCount->value();

	ui->progressBar->setVisible(false);
}

SchedulerWidget::~SchedulerWidget()
{
    delete ui;
}

void SchedulerWidget::progressStarted()
{
	ui->progressBar->setVisible(true);
}

void SchedulerWidget::progressDone()
{
	ui->progressBar->setVisible(false);
}

void SchedulerWidget::changeRenderCount(int value)
{
	s->property["renderCount"] = value;
}