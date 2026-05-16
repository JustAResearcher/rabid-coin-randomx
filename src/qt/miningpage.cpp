#include "miningpage.h"
#include "ui_miningpage.h"
#include <QProcess>
#include <QTimer>
#include <QDir>
#include <QCoreApplication>
#include <QRegExp>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QFile>
#include <QStandardPaths>

MiningPage::MiningPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MiningPage),
    minerProcess(nullptr),
    stratumProcess(nullptr),
    daemonProcess(nullptr),
    acceptedShares(0),
    connectedWorkers(0)
{
    ui->setupUi(this);

    statsTimer = new QTimer(this);
    connect(statsTimer, SIGNAL(timeout()), this, SLOT(updateStats()));

    connect(ui->startMiningButton, SIGNAL(clicked()), this, SLOT(startMining()));
    connect(ui->stopMiningButton, SIGNAL(clicked()), this, SLOT(stopMining()));
    connect(ui->startStratumButton, SIGNAL(clicked()), this, SLOT(startStratum()));
    connect(ui->stopStratumButton, SIGNAL(clicked()), this, SLOT(stopStratum()));
    connect(ui->stopDaemonButton, SIGNAL(clicked()), this, SLOT(stopDaemon()));
    connect(ui->startDaemonButton, SIGNAL(clicked()), this, SLOT(startDaemon()));
    connect(ui->threadSlider, SIGNAL(valueChanged(int)), this, SLOT(onThreadSliderChanged(int)));

    // Detect and show local IP for HiveOS
    QString localIP = "YOUR_IP";
    QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
    for (int i = 0; i < ifaces.size() && localIP == "YOUR_IP"; i++) {
        QList<QNetworkAddressEntry> entries = ifaces[i].addressEntries();
        for (int j = 0; j < entries.size(); j++) {
            QHostAddress addr = entries[j].ip();
            if (addr.protocol() == QAbstractSocket::IPv4Protocol &&
                !addr.isLoopback() &&
                !addr.toString().startsWith("169.254")) {
                localIP = addr.toString();
                break;
            }
        }
    }
    localIPStr = localIP;
    ui->localIPLabel->setText("Your IP: " + localIP + " | HiveOS Pool: " + localIP + ":3333");
    // TODO(randomx): PoW is now RandomXv2 (algo "rx/0"); stock xmrig works.
    ui->miningLog->append("=== RabidCoin Mining (RandomXv2) ===");
    ui->miningLog->append("Public Pool: stratum.rabidmining.com:3333");
    ui->miningLog->append("Your Local Stratum: " + localIP + ":3333");
    ui->miningLog->append("Steps: 1) Start Daemon  2) Start Stratum  3) Start Mining");
}

MiningPage::~MiningPage()
{
    if (minerProcess && minerProcess->state() == QProcess::Running) {
        minerProcess->kill();
        minerProcess->waitForFinished(1000);
    }
    if (stratumProcess && stratumProcess->state() == QProcess::Running) {
        stratumProcess->kill();
        stratumProcess->waitForFinished(1000);
    }
    if (daemonProcess && daemonProcess->state() == QProcess::Running) {
        daemonProcess->kill();
        daemonProcess->waitForFinished(1000);
    }
    delete ui;
}

void MiningPage::onThreadSliderChanged(int value)
{
    ui->threadCount->setText(QString::number(value));
}

void MiningPage::startDaemon()
{
    // First check if daemon is already running via RPC
    QString dataDir = ui->dataDirEdit->text().trimmed();
    QString appDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_WIN
    QString cliPath = appDir + "/rabidcoin-cli.exe";
#else
    QString cliPath = appDir + "/rabidcoin-cli";
#endif

    QProcess testRpc;
    QStringList cliArgs;
    if (!dataDir.isEmpty())
        cliArgs << "-datadir=" + dataDir;
    cliArgs << "-testnet" << "-rpcuser=rabiduser" << "-rpcpassword=rabidpass123" << "-rpcport=17332" << "getblockcount";
    testRpc.start(cliPath, cliArgs);
    testRpc.waitForFinished(3000);
    if (testRpc.exitCode() == 0) {
        QString blocks = QString::fromUtf8(testRpc.readAllStandardOutput()).trimmed();
        ui->daemonStatus->setText("Running - Block " + blocks);
        ui->startDaemonButton->setEnabled(false);
        ui->stopDaemonButton->setEnabled(true);
        ui->miningLog->append("Daemon already running! Block height: " + blocks);
        return;
    }

    // RPC not responding - try launching rabidcoind
    if (daemonProcess && daemonProcess->state() == QProcess::Running) {
        ui->miningLog->append("Daemon process already started.");
        return;
    }

#ifdef Q_OS_WIN
    QString daemonPath = appDir + "/rabidcoind.exe";
#else
    QString daemonPath = appDir + "/rabidcoind";
#endif

    if (!QFile::exists(daemonPath)) {
        ui->miningLog->append("rabidcoind not found at " + daemonPath);
        ui->miningLog->append("If using Qt wallet, daemon is built-in - try again in a moment.");
        ui->daemonStatus->setText("Built-in (loading...)");
        return;
    }

    daemonProcess = new QProcess(this);
    connect(daemonProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(onDaemonOutput()));
    connect(daemonProcess, SIGNAL(readyReadStandardError()), this, SLOT(onDaemonOutput()));

    QStringList args;
    if (!dataDir.isEmpty())
        args << "-datadir=" + dataDir;
    args << "-testnet"
         << "-server"
         << "-rpcuser=rabiduser"
         << "-rpcpassword=rabidpass123"
         << "-rpcport=17332"
         << "-port=17333"
         << "-addnode=194.163.150.15"
         << "-listen=1";

    daemonProcess->start(daemonPath, args);

    if (daemonProcess->waitForStarted(3000)) {
        ui->daemonStatus->setText("Running");
        ui->startDaemonButton->setText("Daemon Running");
        ui->startDaemonButton->setEnabled(false);
        ui->stopDaemonButton->setEnabled(true);
        ui->miningLog->append("Daemon started! Syncing with testnet...");
        ui->miningLog->append("Seed node: 194.163.150.15");
    } else {
        ui->miningLog->append("Failed to start daemon.");
        ui->daemonStatus->setText("Failed");
    }
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

    QString appDir = QCoreApplication::applicationDirPath();

    // TODO(randomx): chain switched from GR-Rabid to RandomXv2. Stock xmrig
    // supports algo rx/0 out of the box; legacy xmrig-rabid is no longer needed.
    #ifdef Q_OS_WIN
        QString minerPath = appDir + "/xmrig.exe";
    #else
        QString minerPath = appDir + "/xmrig";
    #endif

    if (!QFile::exists(minerPath)) {
        ui->miningLog->append("Error: xmrig not found at " + minerPath);
        ui->miningLog->append("Place xmrig (RandomX-capable) in the same folder as the wallet.");
        return;
    }

    minerProcess = new QProcess(this);
    connect(minerProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(onMinerOutput()));
    connect(minerProcess, SIGNAL(readyReadStandardError()), this, SLOT(onMinerOutput()));

    QStringList args;
    args << "-o" << pool
         << "-a" << "rx/0"
         << "-u" << user
         << "-p" << "x"
         << "--threads=" + QString::number(threads)
         << "--no-color"
         << "--donate-level=0";

    minerProcess->start(minerPath, args);

    if (minerProcess->waitForStarted(3000)) {
        ui->miningStatus->setText("Mining...");
        ui->startMiningButton->setEnabled(false);
        ui->stopMiningButton->setEnabled(true);
        ui->miningLog->append("Miner started with " + QString::number(threads) + " threads");
        ui->miningLog->append("Pool: " + pool);
        statsTimer->start(5000);
    } else {
        ui->miningLog->append("Failed to start miner!");
    }
}

void MiningPage::stopMining()
{
    if (minerProcess && minerProcess->state() == QProcess::Running) {
        minerProcess->kill();
        minerProcess->waitForFinished(3000);
    }
    minerProcess = nullptr;
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

    QString appDir = QCoreApplication::applicationDirPath();

    // Look for stratum-server.py in app dir
    QString stratumScript = appDir + "/stratum-server.py";

    // Pick python executable - embedded on Windows, python3 on Linux
    #ifdef Q_OS_WIN
        QString pythonPath = appDir + "/python/python.exe";
        if (!QFile::exists(pythonPath)) {
            pythonPath = "python";  // fallback to system python
        }
    #else
        QString pythonPath = "python3";
    #endif

    if (!QFile::exists(stratumScript)) {
        ui->miningLog->append("Error: stratum-server.py not found at " + stratumScript);
        ui->miningLog->append("Place stratum-server.py in the same folder as the wallet.");
        return;
    }

    stratumProcess = new QProcess(this);
    connect(stratumProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(onStratumOutput()));
    connect(stratumProcess, SIGNAL(readyReadStandardError()), this, SLOT(onStratumOutput()));

    QStringList args;
    args << stratumScript;

    stratumProcess->start(pythonPath, args);

    if (stratumProcess->waitForStarted(3000)) {
        ui->stratumStatus->setText("Running on port 3333");
        ui->startStratumButton->setText("Stop Stratum Server");
        ui->stopStratumButton->setEnabled(true);
        ui->miningLog->append("Stratum server started on port 3333");
        ui->miningLog->append("HiveOS rigs connect to: " + localIPStr + ":3333");
        ui->miningLog->append("Use your RABID address as username in HiveOS");
    } else {
        ui->miningLog->append("Failed to start stratum server!");
        ui->miningLog->append("Python path tried: " + pythonPath);
    }
}

void MiningPage::stopStratum()
{
    if (stratumProcess && stratumProcess->state() == QProcess::Running) {
        stratumProcess->kill();
        stratumProcess->waitForFinished(3000);
    }
    stratumProcess = nullptr;
    ui->stratumStatus->setText("Stopped");
    ui->startStratumButton->setText("Start Stratum Server");
    ui->stopStratumButton->setEnabled(false);
}


void MiningPage::stopDaemon()
{
    if (daemonProcess && daemonProcess->state() == QProcess::Running) {
        daemonProcess->kill();
        daemonProcess->waitForFinished(3000);
    }
    daemonProcess = nullptr;
    ui->daemonStatus->setText("Stopped");
    ui->startDaemonButton->setText("Start Daemon");
    ui->startDaemonButton->setEnabled(true);
    ui->stopDaemonButton->setEnabled(false);
    ui->miningLog->append("Daemon stopped.");
}

void MiningPage::updateStats()
{
    ui->connectedWorkers->setText(QString::number(connectedWorkers));
}

void MiningPage::onMinerOutput()
{
    QByteArray output = minerProcess->readAllStandardOutput();
    output += minerProcess->readAllStandardError();
    QString text = QString::fromUtf8(output);

    QRegExp rxHash("(\\d+\\.?\\d*)\\s*(H|KH|MH|GH)/s");
    if (rxHash.indexIn(text) != -1) {
        ui->hashrate->setText(rxHash.cap(1) + " " + rxHash.cap(2) + "/s");
    }

    QRegExp rxAccepted("accepted \\((\\d+)/");
    if (rxAccepted.indexIn(text) != -1) {
        acceptedShares = rxAccepted.cap(1).toInt();
        ui->acceptedShares->setText(QString::number(acceptedShares));
    }

    // TODO(randomx): old filter dropped "GR_RABID:" debug lines from xmrig-rabid;
    // stock xmrig emits "randomx" prefix instead. Verify filter still suppresses
    // the right noise once a real RandomX-capable build is wired up.
    QStringList lines = text.split("\n");
    for (const QString &line : lines) {
        if (!line.contains("randomx ") && !line.trimmed().isEmpty()) {
            ui->miningLog->append(line.trimmed());
        }
    }
}

void MiningPage::onDaemonOutput()
{
    QByteArray output = daemonProcess->readAllStandardOutput();
    output += daemonProcess->readAllStandardError();
    QString text = QString::fromUtf8(output);
    if (!text.trimmed().isEmpty())
        ui->miningLog->append("[daemon] " + text.trimmed());
}

void MiningPage::onStratumOutput()
{
    QByteArray output = stratumProcess->readAllStandardOutput();
    output += stratumProcess->readAllStandardError();
    QString text = QString::fromUtf8(output);

    QRegExp rxWorker("Miner connected");
    int pos = 0;
    while ((pos = rxWorker.indexIn(text, pos)) != -1) {
        connectedWorkers++;
        pos += rxWorker.matchedLength();
    }

    if (!text.trimmed().isEmpty())
        ui->miningLog->append("[stratum] " + text.trimmed());
}
