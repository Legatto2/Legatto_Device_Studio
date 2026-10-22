#include "class_logfile.h"

Class_LogFile::Class_LogFile(QObject *parent) : QObject(parent)
{
    appPath = QDir::currentPath();
    //qDebug()<<" appPath Path: "<<appPath;
    logDir.setPath(QString(appPath+"/Log/"));
    if(!logDir.exists()) logDir.mkdir(QString(appPath+"/Log/"));

    filePath = QString(QString(appPath+"/Log/LegacyServiceClient_LogFile_"+QDate::currentDate().toString("dd:MM:yyyy").replace(":","_")+".txt"));

    //()<<" Log File Path: "<<filePath;
}


void Class_LogFile::rx_initialFileFolders()
{
    //qDebug()<<"rx_initialFileFolders ";
    if(!QFile::exists(filePath)){
        //qDebug()<<"rx_initialFileFolders ";
        logFile = new QFile(filePath);
        if(logFile->open(QIODevice::WriteOnly))
        {
           // qDebug()<<" New File is generated ";
            QByteArray baa("Legatto Device Studio — unsupported legacy client \n +++++++++++++++++++++++++++++++++++++++++++ \n");
            logFile->write(baa);
            logFile->close();
        }
    }
    else
    {
        logFile = new QFile(filePath);
    }

}

void Class_LogFile::writeToLog(QByteArray ba)
{
    if(logFile->open(QIODevice::Append))
    {
        QByteArray info("System Time: ");
        info.append(QTime::currentTime().toString("HH:mm:ss").toUtf8());
        info.append("\n");
        logFile->write(info);

        logFile->write(ba);
        logFile->write("\n========================================================\n\n");
        logFile->close();
    }


    // Remote log upload is disabled until consent, redaction, retention,
    // and per-operation IDs are implemented. Existing local logs are retained.
}
