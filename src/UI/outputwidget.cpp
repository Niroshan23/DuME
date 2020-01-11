#include "outputwidget.h"

OutputWidget::OutputWidget(FFmpeg *ff, int id, MediaList *inputMedias, QWidget *parent) :
    QWidget(parent)
{
    _freezeUI = true;
    _loadingPreset = false;

    setupUi(this);

    // FFmpeg
    _ffmpeg = ff;
    connect( _ffmpeg,SIGNAL(binaryChanged(QString)),this,SLOT(ffmpeg_init()) );
    // Keep the id
    _index = id;
    // Associated MediaInfo
    _mediaInfo = new MediaInfo( _ffmpeg, this);
    connect( _mediaInfo, SIGNAL(changed()), this, SLOT(updateBlocksAvailability()));

    // Input medias
    _inputMedias = inputMedias;
    //connect them
    foreach( MediaInfo *media, _inputMedias->medias())
    {
        connect( media, SIGNAL(changed()), this, SLOT(inputChanged()));
    }
    //get current info
    // TODO update from media

    // CREATE MENUS
    blocksMenu = new QMenu(this);
    blocksMenu->setTearOffEnabled(true);
    addBlockButton->setMenu( blocksMenu );

    presetsMenu = new QMenu(this);
    presetsButton->setMenu( presetsMenu ); 

    // SHADOWS
    videoWidget->setGraphicsEffect( new DropShadow() );
    audioWidget->setGraphicsEffect( new DropShadow() );

    // CREATE BLOCKS
    blocksMenu->addAction(actionVideo);
    blockResize = addBlock( new BlockResize( _mediaInfo ), actionResize );
    blockFrameRate = addBlock( new BlockFrameRate( _mediaInfo ), actionFrameRate );
    blockVideoCodec = addBlock( new BlockVideoCodec( _ffmpeg, _mediaInfo ), actionVideoCodec );
    blockVideoBitrate = addBlock( new BlockVideoBitrate( _mediaInfo ), actionVideoBitrate );
    blockVideoProfile = addBlock( new BlockVideoProfile( _mediaInfo ), actionProfile );
    blockLoops = addBlock( new BlockLoops( _mediaInfo ), actionLoops );
    blockStartNumber = addBlock( new BlockStartNumber( _mediaInfo ), actionStartNumber );
    blockAlpha = addBlock( new BlockAlpha( _mediaInfo ), actionAlpha );
    blockColor = addBlock( new BlockColor( _mediaInfo ), actionColor );
    blockPixFormat = addBlock( new BlockPixFormat( _mediaInfo ), actionPixelFormat );
    blocksMenu->addAction(actionAudio);
    blockSampling = addBlock( new BlockSampling( _mediaInfo ), actionSampling );
    blockAudioCodec = addBlock( new BlockAudioCodec( _ffmpeg, _mediaInfo ), actionAudioCodec );
    blockAudioBitrate = addBlock( new BlockAudioBitrate( _mediaInfo ), actionAudioBitrate );
    blocksMenu->addAction( actionOther );
    blockMap = addBlock( new BlockMapping( _mediaInfo, _inputMedias ), actionMap );
    blocksMenu->addAction( actionAddCustom );

    // PRESETS MENU
    presetsMenu->addAction( actionDefaultPreset );
    presetsMenu->addSeparator();
    presetsMenu->addAction( actionOpenPreset );
    presetsMenu->addAction( actionSavePreset );

    ffmpeg_init();

    _freezeUI = false;

    //Set defaults
    on_presetsFilterBox_activated("");
    //choose preset
    selectDefaultPreset();

    //splitter sizes
    QList<int>sizes;
    sizes << 350;
    sizes << 600;
    splitter->setSizes(sizes);
}

void OutputWidget::ffmpeg_init()
{
    _freezeUI = true;
    _mediaInfo->reInit( false );
    ffmpeg_loadMuxers();
    _freezeUI = false;
}

void OutputWidget::ffmpeg_loadMuxers()
{
    _freezeUI = true;

    formatsBox->clear();

    QList<FFMuxer *> muxers = _ffmpeg->muxers();
    if (muxers.count() == 0) return;

    int formatsFilter = formatsFilterBox->currentIndex();

    foreach(FFMuxer *muxer,muxers)
    {
        //skip muxers without extension
        if ((formatsFilter != 0 && formatsFilter != 5) && muxer->extensions().count() == 0) continue;
        else if (formatsFilter == 5 && muxer->extensions().count() == 0) formatsBox->addItem(muxer->prettyName(),QVariant(muxer->name()));

        if (formatsFilter == 0 ||
                (formatsFilter == 1 && muxer->isAudio() && muxer->isVideo() )  ||
                (formatsFilter == 2 && muxer->isSequence() ) ||
                (formatsFilter == 3 && muxer->isAudio() && !muxer->isVideo() )  ||
                (formatsFilter == 4 && muxer->isVideo() && !muxer->isAudio() ))
        {
            formatsBox->addItem("." + muxer->extensions().join(", .") + " | " + muxer->prettyName(),QVariant(muxer->name()));
        }
    }
    _freezeUI = false;
    on_formatsBox_currentIndexChanged(formatsBox->currentIndex());
}

MediaInfo *OutputWidget::getMediaInfo()
{
    //ADD CUSTOM PARAMS
    foreach ( BlockBaseWidget *b, _customParams )
    {
        BlockCustom *bc = static_cast<BlockCustom *>( b->content() );
        QString param = bc->param();
        if (param != "")
        {
            QStringList option(param);
            option << bc->value();
            _mediaInfo->addFFmpegOption(option);
        }
    }
    MediaInfo *mi = new MediaInfo( _ffmpeg );
    mi->updateInfo( _mediaInfo, true, true);
    return mi;
}

void OutputWidget::setMediaInfo(MediaInfo *mediaInfo)
{
    if (mediaInfo == nullptr) return;

    _mediaInfo->updateInfo( mediaInfo );
}

QString OutputWidget::getOutputPath()
{
    return outputEdit->text();
}

void OutputWidget::updateBlocksAvailability()
{
    //Audio / Video Buttons
    FFMuxer *m = _mediaInfo->muxer();
    if ( m != nullptr )
    {
        bool audio = m->isAudio() && !m->isSequence();
        audioButton->setChecked( audio && _mediaInfo->hasAudio() );
        audioButton->setEnabled ( audio );
        audioTranscodeButton->setEnabled( audio && _mediaInfo->hasAudio() );
        audioCopyButton->setEnabled( audio && _mediaInfo->hasAudio() );

        bool video =  m->isVideo() || m->isSequence();
        videoButton->setChecked( video && _mediaInfo->hasVideo() );
        videoButton->setEnabled(  video  );
        videoTranscodeButton->setEnabled( video && _mediaInfo->hasVideo() );
        videoCopyButton->setEnabled( video && _mediaInfo->hasVideo() );
    }

    //Customs
    //remove previous
    qDeleteAll( _customParams );
    _customParams.clear();
    //add others
    foreach( QStringList option, _mediaInfo->ffmpegOptions() )
    {
        addNewParam( option[0], option[1] );
    }

    //Muxer
    if ( m != nullptr)
    {
        bool found = false;
        for (int i = 0; i < formatsBox->count(); i++)
        {
            if (formatsBox->itemData(i).toString() == m->name())
            {
                formatsBox->setCurrentIndex(i);
                found = true;
                break;
            }
        }
        if (!found)
        {
            //try without filter
            formatsFilterBox->setCurrentIndex(0);
            for (int i = 0; i < formatsBox->count(); i++)
            {
                if (formatsBox->itemData(i).toString() == m->name())
                {
                    formatsBox->setCurrentIndex(i);
                    found = true;
                    break;
                }
            }
        }
    }
    else
    {
        formatsBox->setCurrentIndex(-1);
    }

    //set the preset to custom if we're not loading a preset
    if (!_loadingPreset)
    {
        presetsBox->setCurrentIndex(0);
    }

    mediaInfoEdit->setPlainText( _mediaInfo->getDescription() );
}

void OutputWidget::customParamActivated(bool activated)
{
    if (!activated)
    {
        for ( int i = 0; i < _customParams.count(); i++)
        {
            if (sender() == _customParams[i])
            {
                QWidget *w = _customParams.takeAt(i);
                w->deleteLater();
                return;
            }
        }
    }
}

void OutputWidget::on_videoButton_clicked(bool checked)
{
    videoTranscodeButton->setEnabled( checked );
    videoCopyButton->setEnabled( checked );

    _mediaInfo->setVideo( checked );
}

void OutputWidget::on_audioButton_clicked(bool checked)
{
    audioTranscodeButton->setEnabled( checked );
    audioCopyButton->setEnabled( checked );

    _mediaInfo->setAudio( checked );
}

void OutputWidget::on_videoTranscodeButton_toggled( bool checked )
{
    if (!_loadingPreset) presetsBox->setCurrentIndex(0);

    if ( checked ) _mediaInfo->setVideoCodec( nullptr );
    else _mediaInfo->setVideoCodec( "copy" );
}

void OutputWidget::on_audioTranscodeButton_toggled( bool checked )
{
    if (!_loadingPreset) presetsBox->setCurrentIndex(0);

    if ( checked ) _mediaInfo->setAudioCodec( nullptr );
    else _mediaInfo->setAudioCodec( "copy" );
}

void OutputWidget::on_outputBrowseButton_clicked()
{
    QString outputPath = QFileDialog::getSaveFileName(this,"Output file",outputEdit->text());
    if (outputPath == "") return;
    updateOutputExtension(outputPath);
}

void OutputWidget::on_formatsBox_currentIndexChanged(int index)
{
    if (index == -1) return;
    FFMuxer *_currentMuxer = _ffmpeg->muxer(formatsBox->currentData().toString());

    _mediaInfo->setMuxer( _currentMuxer );

    if (_freezeUI) return;
    _freezeUI = true;

    //UI

    //video
    if (_currentMuxer->isVideo())
    {
        videoButton->setChecked(true);
        videoTranscodeButton->setEnabled(true);
        videoCopyButton->setEnabled(true);
    }
    else
    {
        videoButton->setChecked(false);
        videoTranscodeButton->setEnabled(false);
        videoCopyButton->setEnabled(false);
    }

    //audio
    if (_currentMuxer->isAudio())
    {
        audioButton->setChecked( true );
        audioTranscodeButton->setEnabled(true);
        audioCopyButton->setEnabled(true);
    }
    else
    {
        audioButton->setChecked( false );
        audioTranscodeButton->setEnabled(false);
        audioCopyButton->setEnabled(false);
    }



    _freezeUI = false;

    updateOutputExtension(outputEdit->text());
    if (!_loadingPreset) presetsBox->setCurrentIndex(0);
}

void OutputWidget::on_formatsFilterBox_currentIndexChanged(int index)
{
    qDebug() << "Format filter selected: " + QString::number(index);
    ffmpeg_loadMuxers();
}

void OutputWidget::on_presetsBox_currentIndexChanged(int index)
{
    if (index == 0) return;
    if (_freezeUI) return;

    _freezeUI = true;
    actionDefaultPreset->setChecked( presetsBox->itemData(index).toString() == settings.value("presets/default",":/presets/MP4 - Auto - Normal").toString());
    _freezeUI = false;

    //load
    _loadingPreset = true;
    //load
    _mediaInfo->loadPreset(presetsBox->itemData(index).toString());
    _loadingPreset = false;
}

void OutputWidget::on_presetsFilterBox_activated(QString arg1)
{
    qDebug() << "Preset filter selected: " + arg1;
    loadPresets( );
}

void OutputWidget::updateOutputExtension(QString outputPath)
{
    if (outputPath == "") return;
    QFileInfo output(outputPath);

    //remove existing {#####}
    QString outputName = output.completeBaseName();
    outputName = outputName.replace(QRegularExpression("_?{#+}"),"");

    QString newExt = "";

    FFMuxer *_currentMuxer = _mediaInfo->muxer();

    if (_currentMuxer != nullptr)
    {
        //add {#####}
        if (_currentMuxer->isSequence())
        {
            newExt = "_{#####}";
        }

        if (_currentMuxer->extensions().count() > 0)
        {
            newExt += "." + _currentMuxer->extensions()[0];
        }

    }
    QString num = "";
    if (_index > 1) num = "_" + QString::number(_index);
    outputName.replace(QRegularExpression(num + "$"),"");
    outputPath = output.path() + "/" + outputName + num + newExt;
    //check if file exists
    if (QFile(outputPath).exists())
    {
        outputPath = output.path() + "/" + outputName + "_transcoded" + num + newExt;
    }

    outputEdit->setText(QDir::toNativeSeparators(outputPath));

    _mediaInfo->setFileName( outputEdit->text() );
}

void OutputWidget::addNewParam(QString name, QString value)
{
    //add a param and a value
    qDebug() << "New Custom param: " + name + " " + value;
    BlockCustom *block = new BlockCustom( _mediaInfo, name, value );
    BlockBaseWidget *bw = new BlockBaseWidget( "FFmpeg parameter", block, blocksWidget );
    blocksLayout->addWidget( bw );
    bw->show();
    connect( bw, SIGNAL(activated(bool)), this, SLOT(customParamActivated(bool)));
    //add to list
    _customParams << bw;
}

void OutputWidget::inputChanged()
{

    //TODO Replace by input monitoring by blocks themselves

    MediaInfo *input = static_cast<MediaInfo *>(sender());

    //set output fileName
    QFileInfo inputFile(input->fileName());
    QString outputPath = inputFile.path() + "/" + inputFile.completeBaseName();
    updateOutputExtension(outputPath);

    //update blocks availability
    updateBlocksAvailability();

    /*// update (hidden) fields
    if (blockResize->isHidden() && input->videoStreams().count() > 0)
    {
        blockResizeContent->setWidth( input->videoStreams()[0]->width() );
        blockResizeContent->setHeight( input->videoStreams()[0]->height() );
    }
    if (blockFrameRate->isHidden() && input->videoStreams().count() > 0) blockFrameRateContent->setFrameRate( input->videoStreams()[0]->framerate() );

    if ( !input->hasAlpha() )
    {
        actionAlpha->setVisible(false);
        blockAlpha->hide();
    }

    if (blockSampling->isHidden() && input->audioStreams().count() > 0) blockSamplingContent->setSampling( input->audioStreams()[0]->samplingRate() );
*/
    //If ae render queue
    this->setVisible( !(input->isAep() && input->aeUseRQueue()));
}

void OutputWidget::loadPresets()
{
    if (_freezeUI) return;
    _freezeUI = true;

    presetsBox->clear();
    int index = presetsFilterBox->currentIndex();

    QStringList presets;
    //list internal
    if (index == 0 || index == 1)
    {
        foreach(QString preset, QDir(":/presets/").entryList(QDir::Files))
        {
            presets << ":/presets/" + preset;
        }
    }
    //list users
    if (index == 0 || index == 2)
    {
        QString userPresetsPath = settings.value("presets/path",QDir::homePath() + "/DuME Presets/").toString();
        QStringList filters("*.meprst");
        filters << "*.json" << "*.dffp";
        foreach (QString preset, QDir(userPresetsPath).entryList(filters,QDir::Files))
        {
            presets << userPresetsPath + "/" + preset;
        }
    }

    presets.sort();
    //add custom
    presetsBox->addItem("Custom");
    foreach(QString preset,presets)
    {
        presetsBox->addItem(QFileInfo(preset).completeBaseName(),preset);
    }

    _freezeUI = false;
}

void OutputWidget::selectDefaultPreset()
{
    QString defaultPreset = settings.value("presets/default",":/presets/MP4 - Auto - Normal").toString();
    for (int i = 0; i < presetsBox->count(); i++)
    {
        if (presetsBox->itemData(i).toString() == defaultPreset)
        {
            presetsBox->setCurrentIndex(i);
            _freezeUI = true;
            actionDefaultPreset->setChecked(true);
            _freezeUI = false;
            break;
        }
    }
}

BlockBaseWidget *OutputWidget::addBlock(BlockContentWidget *content, QAction *action )
{
    // create block
    BlockBaseWidget *b = new BlockBaseWidget( action->text(), content, blocksWidget);
    blocksLayout->addWidget( b );
    //add and connect action
    blocksMenu->addAction( action );
    connect( action, SIGNAL( triggered(bool) ), b, SLOT( activate(bool) ) );
    connect( b, SIGNAL( activated(bool) ), action, SLOT( setChecked( bool ) ) );
    connect( b, SIGNAL( blockEnabled(bool) ), action, SLOT(setVisible(bool)));

    return b;
}

void OutputWidget::on_actionAddCustom_triggered()
{
    addNewParam();
}

void OutputWidget::on_actionSavePreset_triggered()
{
    _freezeUI = true;

    presetsBox->setCurrentIndex(0);

    //ask for file
    QString saveFileName = QFileDialog::getSaveFileName(this,"Save output preset",settings.value("presets/path",QDir::homePath() + "/DuME Presets/").toString(),"DuME preset (*.meprst);;JSON (*.json);;All Files (*.*)");
    if (saveFileName == "")
    {
        _freezeUI = false;
        return;
    }

    //add extension if none
    if ( QFileInfo(saveFileName).suffix() == "" ) saveFileName += ".meprst";

    //update infos
    getMediaInfo();
    //export
    _mediaInfo->exportPreset(saveFileName);
    //add to box
    _freezeUI = false;
    loadPresets();
}

void OutputWidget::on_actionOpenPreset_triggered()
{
    _freezeUI = true;

    presetsBox->setCurrentIndex(0);

    QString openFileName = QFileDialog::getOpenFileName(this,"Load output preset",settings.value("presets/path",QDir::homePath() + "/DuME Presets/").toString(),"DuME preset (*.meprst);;JSON (*.json);;All Files (*.*)");
    if (openFileName == "")
    {
        _freezeUI = false;
        return;
    }
    _loadingPreset = true;
    _freezeUI = false;
    //load
    _mediaInfo->loadPreset(openFileName);
    _loadingPreset = false;
}

void OutputWidget::on_actionDefaultPreset_triggered(bool checked)
{
    if (_freezeUI) return;
    if (checked) settings.setValue("presets/default", presetsBox->currentData().toString());
    else settings.setValue("presets/default", ":/presets/MP4 - Auto - Normal");
}