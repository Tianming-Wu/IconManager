#include "mgrmainwindow.h"
#include "./ui_mgrmainwindow.h"

#include <QDebug>

MgrMainWindow::MgrMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MgrMainWindow)
{
    ui->setupUi(this);

    uid_t realuid = geteuid();
    isroot = (realuid == 0);

    if(isroot) {
        this->setWindowTitle("IconManager (root)");
    } else {
        // ui->btnRoot->setVisible(true);
        // connect(ui->btnRoot, &QPushButton::clicked, this, &MgrMainWindow::restartAsRoot);
    }

    connect(ui->btnEdit, &QPushButton::clicked, this, &MgrMainWindow::startEdit);

    connect(ui->checkFilterRegex, &QPushButton::toggled, this, [=](bool checked){ filterRegexEnabled = checked; RefreshDisplay(); });
    connect(ui->editFilter, &QLineEdit::textEdited, this, [=](const QString &str){ filter = str; RefreshDisplay(); });

    connect(ui->listIcons, &QListWidget::currentItemChanged, this ,&MgrMainWindow::listIcons_selectionChanged);

    ui->labelIconDisplay->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->labelIconDisplay->setScaledContents(true);

    LoadRefresh();
}

MgrMainWindow::~MgrMainWindow()
{
    if(proc != nullptr) {
        proc->write("exit\n");
        proc->waitForFinished(500);
    }

    delete ui;
}

void MgrMainWindow::startEdit()
{
    QInputDialog diag(this);
    QString rootpwd;

    diag.setInputMethodHints(Qt::ImhHiddenText);
    int res = diag.exec();
    if(res == QInputDialog::Accepted) {
        rootpwd = diag.textValue();
    } else return;

    ui->btnEdit->setEnabled(false);

    if(proc == nullptr) {
        proc = new QProcess(this);
        proc->setProgram("sudo");
        proc->setArguments({QString("./IconMgrBkgnd")});
    }

    ui->labelEdit->setText("后端启动中");
    proc->start();

    if(!proc->waitForStarted(5000)) {
        ui->labelEdit->setText("<html><head/><body><p><span style=\" color:#ed333b;\">后端进程启动失败</span></p></body></html>");
    }
}

void MgrMainWindow::LoadRefresh()
{
    icomap.clear();

    // main scan process
    for(const auto &entry : fs::directory_iterator("/usr/share/icons/hicolor")) {
        if(entry.is_directory()) {
            std::string size, filename = entry.path().filename().string();
            if(filename.find('x') != std::string::npos) {
                size = filename.substr(0, filename.find('x'));
            } else if(filename == "scalable") {
                size = "scalable";
            } else if(filename == "symbolic") {
                size = "symbolic";
            } else continue; // to skip /apps
            QString qsize = QString::fromStdString(size);
            for(const auto &fentry : fs::directory_iterator(entry.path()/"apps")) {
                if(fentry.is_regular_file()) {
                    std::string name = fentry.path().filename().stem().string();
                    QString qname = QString::fromStdString(name);
                    if(!icomap.contains(qname)) { // not exists
                        IconInfo ii;
                        ii.sizes.push_back(qsize);
                        ii.is_symlink = fentry.is_symlink();
                        icomap.insert(qname, ii);
                    } else { // exists, add new sizes
                        icomap[qname].sizes.push_back(qsize);
                    }
                }
            }
        }
    }
    
    // list as arbitary order (cannot compile)
    // std::sort(icomap.begin(), icomap.end(),
    // [](std::pair<QString,IconInfo> a, std::pair<QString,IconInfo> b) {
    //     return a.first.compare(b.first);
    // });

    QRegularExpression regnum("^\\d*$");
    for(QIconMap::Iterator it = icomap.begin(); it != icomap.end(); it++) {
        // sort the sizes
        std::sort((*it).sizes.begin(), (*it).sizes.end(),
        [=](QString &a, QString &b) {
            if(a.contains(regnum) && !b.contains(regnum)) return true;
            if(!a.contains(regnum) && b.contains(regnum)) return false; // place numbers in the front
            if(!a.contains(regnum) && !b.contains(regnum)) return (a=="scalable");
            // only one case left
            return (a.toInt() < b.toInt());
        });
        // store the display icon
        // fs::path path("/usr/share/icons/hicolor");
        // QString qsmallest = (*it).sizes[0];
        // std::string smallest = qsmallest.toStdString();
        // if(qsmallest.contains(regnum)) {
        //     path = path/(smallest+"x"+smallest)/"apps"/(it.key().toStdString()+".png");
        // } else if(qsmallest == "symbolic") {
        //     path = path/(smallest)/"apps"/(it.key().toStdString()+".png");
        // } else { // scalable
        //     path = path/(smallest)/"apps"/(it.key().toStdString()+".svg");
        // }
        // (*it).icon = QIcon(QString::fromStdString(path.string()));
        // // (*it).iconpath = ;

        fs::path path("/usr/share/icons/hicolor");
        for(QString &qs : (*it).sizes) {
            std::string s = qs.toStdString();
            if(qs.contains(regnum)) {
                path = path/(s+"x"+s)/"apps"/(it.key().toStdString()+".png");
            } else if(qs == "symbolic") {
                path = path/(s)/"apps"/(it.key().toStdString()+".png");
            } else { // scalable
                path = path/(s)/"apps"/(it.key().toStdString()+".svg");
            }
            (*it).icon.addFile(QString::fromStdString(path.string()));
        }

    }
    
    RefreshDisplay();
}

void MgrMainWindow::RefreshDisplay()
{
    ui->listIcons->clear();

    QRegularExpression pattern;

    bool filterEnabled = !filter.isEmpty();
\
    if(filterEnabled && filterRegexEnabled) {
        pattern.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        pattern.setPattern(filter);

        if(!pattern.isValid()) {
            ui->editFilter->setStyleSheet("QLineEdit{color:rgb(255,0,0);}");
            return; // blocks filter from working
        } else ui->editFilter->setStyleSheet("");
    }

    if(!filterEnabled) ui->editFilter->setStyleSheet(""); // If directly deletes everything from a invalid filter

    std::function<void(QIconMap::Iterator&)> addItem = [&](QIconMap::Iterator &it)
    {
        QListWidgetItem *item = new QListWidgetItem(ui->listIcons);
        QVariant v; v.setValue(it.value());
        item->setText(it.key());
        item->setIcon(it.value().icon);
        item->setData(LISTICON_ICONINFO, v);
        ui->listIcons->addItem(item);
    };

    for(QIconMap::Iterator it = icomap.begin(); it != icomap.end(); it++) {
        if(filterEnabled) {
            if(filterRegexEnabled) {
                if(it.key().contains(pattern)) addItem(it);
            } else {
                if(it.key().contains(filter, Qt::CaseInsensitive)) addItem(it);
            }
        } else addItem(it);
    }
}

void MgrMainWindow::listIcons_selectionChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if(current == nullptr) {
        ui->labelSizes->clear();
        ui->labelIconDisplay->setPixmap(QPixmap());
        return;
    }

    QRegularExpression regnum("^\\d*$");
    IconInfo ii = current->data(LISTICON_ICONINFO).value<IconInfo>();
    QStringList l;
    QString lsz; // largest size

    for(const QString& s : ii.sizes) {
        l.push_back(s);
        if(l.contains(regnum)) lsz = s;
        else if(lsz.isEmpty()) lsz = s;
    }
    ui->labelSizes->setText(l.join(","));

    fs::path path("/usr/share/icons/hicolor");
    std::string s = lsz.toStdString();
    if(lsz.contains(regnum)) {
        path = path/(s+"x"+s)/"apps"/(current->text().toStdString()+".png");
    } else if(lsz == "symbolic") {
        path = path/s/"apps"/(current->text().toStdString()+".png");
    } else { // scalable
        path = path/s/"apps"/(current->text().toStdString()+".svg");
    }

    QPixmap pixmap(QString::fromStdString(path.string()));
    ui->labelIconDisplay->setPixmap(pixmap);
}
