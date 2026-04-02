#include "miningpage.h"
#include "ui_miningpage.h"
#include <QProcess>
#include <QTimer>
#include <QDir>
#include <QCoreApplication>
#include <QRegExp>

MiningPage::MiningPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MiningPage),
    minerProcess(nullptr),
    stratumProcess(nullptr),
    acceptedShares(0),
    connectedWorkers(0)
{
    ui->setupUi(this);

    statsTimer = new QTimer(this);
    connect(statsTimer, SIGNAL(timeout()), this, SLOT(updateStats()));

    connect(ui->startMiningButton, SIGNAL(clicked()), this, SLOT(startMining()));
    connect(ui->stopMiningButton, SIGNAL(clicked()), this, SLOT(stopMining()));
    connect(ui->startStratumButton, SIGNAL(clicked()), this, SLOT(startStratum()));
    connect(ui->threadSlider, SIGNAL(valueChanged(int)), this, SLOT(onThreadSliderChanged(int)));
}

MiningPage::~MiningPage()
{
    stopMining();
    stopStratum();
    delete ui;
}

void MiningPage::onThreadSliderChanged(int value)
{
    ui->threadCount->setText(QString::number(value));
}

void MiningPage::startMining()
{
    if (minerProcess && minerProcess->state() == QProcess::Running)
        return;

    acceptedShares = 0;
    ui->acceptedShares->setText("0");

    QString pool = ui->poolAddress->text();
    QString wallet = ui->walletAddress->text();
    QString worker = ui->workerName->text();
    int threads = ui->threadSlider->value();

    if (wallet.isEmpty()) {
        ui->miningLog->append("Error: Please enter your wallet address!");
        return;
    }

    QString user = wallet + "." + worker;

    // Find xmrig-rabid binary next to wallet
    QString appDir = QCoreApplication::applicationDirPath();
    QString minerPath = appDir + "/xmrig-rabid";
    
    #ifdef Q_OS_WIN
        minerPath += ".exe";
    #endif

    if (!QFile::exists(minerPath)) {
        ui->miningLog->append("Error: xmrig-rabid not found at " + minerPath);
        ui->miningLog->append("Please place xmrig-rabid in the same folder as the wallet.");
        return;
    }

    minerProcess = new QProcess(this);
    connect(minerProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(onMinerOutput()));
    connect(minerProcess, SIGNAL(readyReadStandardError()), this, SLOT(onMinerOutput()));

    QStringList args;
    args << "-o" << pool
         << "-a" << "gr-rabid"
         << "-u" << user
         << "-p" << "x"
         << "--threads=" + QString::number(threads)
         << "--no-color";

    minerProcess->start(minerPath, args);

    if (minerProcess->waitForStarted(3000)) {
        ui->miningStatus->setText("Mining...");
        ui->startMiningButton->setEnabled(false);
        ui->stopMiningButton->setEnabled(true);
        ui->miningLog->append("Miner started: " + minerPath);
        statsTimer->start(5000);
    } else {
        ui->miningLog->append("Failed to start miner!");
    }
}

void MiningPage::stopMining()
{
    if (minerProcess) {
        minerProcess->kill();
        minerProcess->waitForFinished(3000);
        delete minerProcess;
        minerProcess = nullptr;
    }
    ui->miningStatus->setText("Not Mining");
    ui->hashrate->setText("0 H/s");
    ui->startMiningButton->setEnabled(true);
    ui->stopMiningButton->setEnabled(false);
    statsTimer->stop();
}

void MiningPage::startStratum()
{
    if (stratumProcess && stratumProcess->state() == QProcess::Running) {
        stopStratum();
        return;
    }

    QString port = ui->stratumPort->text();
    QString appDir = QCoreApplication::applicationDirPath();
    QString stratumPath = appDir + "/rabidcoin-stratum-server.py";

    stratumProcess = new QProcess(this);
    connect(stratumProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(onStratumOutput()));

    QStringList args;
    args << stratumPath;

    stratumProcess->start("python3", args);

    if (stratumProcess->waitForStarted(3000)) {
        ui->stratumStatus->setText("Running on port " + port);
        ui->startStratumButton->setText("Stop Stratum Server");
        ui->miningLog->append("Stratum server started on port " + port);
        ui->miningLog->append("HiveOS rigs can now connect to YOUR_IP:" + port);
    } else {
        ui->miningLog->append("Failed to start stratum server!");
    }
}

void MiningPage::stopStratum()
{
    if (stratumProcess) {
        stratumProcess->kill();
        stratumProcess->waitForFinished(3000);
        delete stratumProcess;
        stratumProcess = nullptr;
    }
    ui->stratumStatus->setText("Stopped");
    ui->startStratumButton->setText("Start Stratum Server");
}

void MiningPage::updateStats()
{
    // Update connected workers from stratum
    ui->connectedWorkers->setText(QString::number(connectedWorkers));
}

void MiningPage::onMinerOutput()
{
    QByteArray output = minerProcess->readAllStandardOutput();
    output += minerProcess->readAllStandardError();
    QString text = QString::fromUtf8(output);

    // Parse hashrate
    QRegExp rxHash("(\d+\.?\d*)\s*(H|KH|MH|GH)/s");
    if (rxHash.indexIn(text) != -1) {
        ui->hashrate->setText(rxHash.cap(1) + " " + rxHash.cap(2) + "/s");
    }

    // Parse accepted shares
    QRegExp rxAccepted("accepted \((\d+)/");
    if (rxAccepted.indexIn(text) != -1) {
        acceptedShares = rxAccepted.cap(1).toInt();
        ui->acceptedShares->setText(QString::number(acceptedShares));
    }

    // Filter out debug lines
    QStringList lines = text.split("\n");
    for (const QString &line : lines) {
        if (!line.contains("GR_RABID:") && !line.trimmed().isEmpty()) {
            ui->miningLog->append(line.trimmed());
        }
    }
}

void MiningPage::onStratumOutput()
{
    QByteArray output = stratumProcess->readAllStandardOutput();
    QString text = QString::fromUtf8(output);
    
    // Count connected workers
    QRegExp rxWorker("Miner connected");
    int pos = 0;
    connectedWorkers = 0;
    while ((pos = rxWorker.indexIn(text, pos)) != -1) {
        connectedWorkers++;
        pos += rxWorker.matchedLength();
    }
    
    ui->miningLog->append(text.trimmed());
}
