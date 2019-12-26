#include "abstractrenderer.h"

AbstractRenderer::AbstractRenderer(QObject *parent) : QObject(parent)
{
    setStatus( MediaUtils::Initializing );

    //set default values
    _currentFrame = 0;
    _numFrames = 0;
    _frameRate = 24;
    _startTime = QTime(0,0,0,0);
    _outputSize = 0;
    _outputBitrate = 0;
    _expectedSize = 0;
    _encodingSpeed = 0;
    _timeRemaining = QTime(0,0,0,0);
    _elapsedTime = QTime(0,0,0,0);

    _outputFileName = "";

    _binaryFileName = "";

    _stopCommand = "";
}

int AbstractRenderer::currentFrame() const
{
    return _currentFrame;
}

QTime AbstractRenderer::startTime() const
{
    return _startTime;
}

double AbstractRenderer::outputSize( ) const
{
    return _outputSize;
}

double AbstractRenderer::outputBitrate( ) const
{
    return _outputBitrate;
}

double AbstractRenderer::encodingSpeed() const
{
    return _encodingSpeed;
}

QTime AbstractRenderer::timeRemaining() const
{
    return _timeRemaining;
}

QString AbstractRenderer::outputFileName() const
{
    return _outputFileName;
}

void AbstractRenderer::setOutputFileName(const QString &outputFileName)
{
    _outputFileName = outputFileName;
}

void AbstractRenderer::setStopCommand(const QString &stopCommand)
{
    _stopCommand = stopCommand;
}

void AbstractRenderer::start( QStringList arguments, int numThreads )
{
    setStatus( MediaUtils::Launching );

    emit newLog("Launching " + QString::number( numThreads ) + " processes.");
    for (int i = 0; i < numThreads; i++ )
    {
        launchProcess( arguments );
    }
    _startTime.start();

    setStatus( MediaUtils::Encoding );
}

void AbstractRenderer::stop(int timeout)
{
    emit newLog("Sending the stop command");

    setStatus( MediaUtils::Cleaning );

    // send the stop command to everyone
    if ( _stopCommand != "")
    {
        foreach( QProcess *renderProcess, _renderProcesses )
        {
            if (renderProcess->state() == QProcess::NotRunning) continue;
            renderProcess->write( _stopCommand.toUtf8() );
        }
    }

    emit newLog("Stop command sent. Waiting for processes to shut down.");
    setStatus( MediaUtils::Stopped );

    // wait for timeout and kill all remaining processes
    QTimer::singleShot(timeout, this, SLOT( killRenderProcesses()) );
}

void AbstractRenderer::processStdError()
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    int id = _renderProcesses.indexOf(process);
    QString log = "Process " + QString::number(id) + ": " + process->readAllStandardError();
    readyRead( log );
    emit newLog( log, LogUtils::Debug );
}

void AbstractRenderer::processStdOutput()
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    int id = _renderProcesses.indexOf(process);
    QString log = "Process " + QString::number(id) + ": " + process->readAllStandardOutput();
    readyRead( log );
    emit newLog( log, LogUtils::Debug );
}

void AbstractRenderer::processStarted()
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    int id = _renderProcesses.indexOf(process);

    emit newLog("Process " + QString::number( id ) + " started.");
}

void AbstractRenderer::processFinished()
{
    //check which processes have finished
    QList<int> finishedProcesses;
    QString debugFinishedProcesses = "Processes: ";
    for(int i = 0; i < _renderProcesses.count(); i++)
    {
        if( _renderProcesses[i]->state() == QProcess::NotRunning )
        {
            finishedProcesses << i;
            if (i > 0) debugFinishedProcesses += ", ";
            debugFinishedProcesses += QString::number(i);
        }
    }

    emit newLog( debugFinishedProcesses + " have finished." );

    //if all processes have finished
    if ( finishedProcesses.count() == _renderProcesses.count() )
    {
        //remove all processes
        while( _renderProcesses.count() > 0 )
        {
            QProcess *process = _renderProcesses.takeLast();
            process->deleteLater();
        }

        setStatus( MediaUtils::Finished );
    }
}

void AbstractRenderer::processErrorOccurred(QProcess::ProcessError e)
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    int id = _renderProcesses.indexOf(process);

    QString error;
    if (e == QProcess::FailedToStart)
    {
        error = "Failed to start process " + QString::number( id ) + ".";
    }
    else if (e == QProcess::Crashed)
    {
        error = "Process (" + QString::number( id ) + ") just crashed.";
    }
    else if (e == QProcess::Timedout)
    {
        error = "Process (" + QString::number( id ) + ") operation timed out.";
    }
    else if (e == QProcess::WriteError)
    {
        error = "Process (" + QString::number( id ) + ") write Error.";
    }
    else if (e == QProcess::ReadError)
    {
        error = "Cannot read process (" + QString::number( id ) + ") output.";
    }
    else if (e == QProcess::UnknownError)
    {
        error = "An unknown process (" + QString::number( id ) + ") error occured.";
    }

    emit newLog( error, LogUtils::Warning );

    // Check if all processes have stopped
    QList<int> finishedProcesses;
    for(int i = 0; i < _renderProcesses.count(); i++)
    {
        if( _renderProcesses[i]->state() == QProcess::NotRunning )
        {
            finishedProcesses << i;
        }
    }

    //if all processes have finished
    if ( finishedProcesses.count() == _renderProcesses.count() )
    {
        //remove all processes
        while( _renderProcesses.count() > 0 )
        {
            QProcess *process = _renderProcesses.takeLast();
            process->deleteLater();
        }

        setStatus( MediaUtils::Error );
    }
}

void AbstractRenderer::killRenderProcesses()
{   
    bool killed = false;
    while ( _renderProcesses.count() > 0 )
    {
        QProcess *rp = _renderProcesses.takeLast();
        if (rp->state() != QProcess::NotRunning)
        {
            rp->kill();
            emit newLog( "Killed process " + QString::number( _renderProcesses.count() + 1 ) );
            killed = true;
        }
        rp->deleteLater();
    }
    if (killed) emit newLog("Some processes did not stop correctly and had to be killed. The output file which may be corrupted.");
}

MediaUtils::Status AbstractRenderer::status() const
{
    return _status;
}

void AbstractRenderer::setBinary(const QString &binaryFileName)
{
    _binaryFileName = binaryFileName;
    setStatus( MediaUtils:: Waiting );
}

void AbstractRenderer::setStatus(MediaUtils::Status status)
{
    _status = status;
    emit statusChanged( _status );
}

double AbstractRenderer::expectedSize() const
{
    return _expectedSize;
}

int AbstractRenderer::numFrames() const
{
    return _numFrames;
}

double AbstractRenderer::frameRate() const
{
    return _frameRate;
}

void AbstractRenderer::setFrameRate(double frameRate)
{
    _frameRate = frameRate;
}

QTime AbstractRenderer::elapsedTime() const
{
    return _elapsedTime;
}

void AbstractRenderer::setNumFrames(int numFrames)
{
    _numFrames = numFrames;
}

void AbstractRenderer::setCurrentFrame(int currentFrame)
{
    _currentFrame = currentFrame;

    if (_currentFrame == 0)
    {
        _outputSize = 0;
        _outputBitrate = 0;
        _encodingSpeed = 0;
        _timeRemaining = QTime( 0, 0 );
        _elapsedTime = QTime( 0, 0 );
        emit progress();
        return;
    }

    //compute times

    QTime currentTime = QTime::currentTime();
    int elapsedSeconds = _startTime.secsTo( currentTime );
    int remainingSeconds = elapsedSeconds * _numFrames / _currentFrame;

    _elapsedTime = QTime( 0, 0 ).addSecs( elapsedSeconds );
    _timeRemaining = QTime( 0, 0 ).addSecs( remainingSeconds );

    //compute size

    //get file(s)
    //check if it's a sequence
    QRegularExpression regExDigits("{(#+)}");
    QRegularExpressionMatch regExDigitsMatch = regExDigits.match( _outputFileName );
    QFileInfo infoOutput( _outputFileName );
    // get all frames
    if ( regExDigitsMatch.hasMatch() )
    {
        QDir containingDir = infoOutput.dir();
        QString baseName = infoOutput.fileName();
        QFileInfoList files = containingDir.entryInfoList( QStringList( baseName.replace( regExDigits, "*") ), QDir::Files);
        _outputSize = 0;
        foreach( QFileInfo file, files)
        {
            _outputSize += file.size();
        }
    }
    //get video file
    else
    {
        _outputSize = infoOutput.size();
    }

    //compute bitrate

    double outputBits = _outputSize / 8;
    double renderedSeconds = _currentFrame / _frameRate;
    _outputBitrate = outputBits / renderedSeconds;

    //expected size
    _expectedSize = _outputSize * _numFrames / _currentFrame;

    //encoding speed
    double expectedDuration = elapsedSeconds + remainingSeconds;
    double videoDuration = _numFrames / _frameRate;
    _encodingSpeed = videoDuration / expectedDuration;
}

void AbstractRenderer::readyRead(QString output)
{
    emit console( output );
    emit progress();
}

void AbstractRenderer::launchProcess( QStringList arguments )
{
    //create process
    QProcess *renderer = new QProcess(this);
    connect( renderer, SIGNAL(readyReadStandardError()), this, SLOT(processStdError()));
    connect( renderer, SIGNAL(readyReadStandardOutput()), this, SLOT(processStdOutput()));
    connect( renderer, SIGNAL(processStarted()), this, SLOT(processStarted()));
    connect( renderer, SIGNAL(processFinished(int)), this, SLOT(processFinished()));
    connect( renderer, SIGNAL(processErrorOccurred(QProcess::ProcessError)), this, SLOT(processErrorOccurred(QProcess::ProcessError)));

    //launch
    renderer->setProgram( _binaryFileName );
    renderer->setArguments( arguments );
    renderer->start(QIODevice::ReadWrite);

    //TODO check processor affinity?

    _renderProcesses << renderer;

    emit newLog("Launched process: " + QString::number( _renderProcesses.count() ));
}
