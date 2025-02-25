#ifndef MGRMAINWINDOW_H
#define MGRMAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QInputDialog>
#include <QListWidgetItem>
#include <QPixmap>
#include <QRegularExpression>
#include <QHash>
#include <QIcon>
#include <QFile>

#include <unistd.h>
#include <filesystem>
#include <functional>
#include <algorithm>

namespace fs = std::filesystem;

QT_BEGIN_NAMESPACE
namespace Ui {
class MgrMainWindow;
}
QT_END_NAMESPACE

class IconInfo
{
public:
    QVector<QString> sizes;
    bool is_symlink = false;
    QIcon icon;
};

Q_DECLARE_METATYPE(IconInfo)

#define LISTICON_ICONINFO 3
// 0 -?, 1 -icon, 2 -text

typedef QHash<QString,IconInfo> QIconMap;

class MgrMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MgrMainWindow(QWidget *parent = nullptr);
    ~MgrMainWindow();

    void LoadRefresh();
    void RefreshDisplay();

public slots:
    void startEdit();
    void listIcons_selectionChanged(QListWidgetItem*, QListWidgetItem*);

private:
    Ui::MgrMainWindow *ui;

    bool isroot;
    QProcess *proc = nullptr;

    QIconMap icomap;

    QString filter;
    bool filterRegexEnabled = false;
};

#endif // MGRMAINWINDOW_H
