/**
 * CalcForge C++ Version - Main Application
 * Advanced Calculator with Mathematical Operations and Unit Conversion
 */

#include <QApplication>
#include <QIcon>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QTimer>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qDebug() << "Starting CalcForge...";

    // Set application properties
    app.setApplicationName("CalcForge");
    app.setApplicationVersion("5.0.0");
    app.setOrganizationName("CalcForge");
    app.setOrganizationDomain("calcforge.org");

    qDebug() << "Creating MainWindow...";

    // Create and show main window
    MainWindow window;

    qDebug() << "Showing window...";
    window.show();

    qDebug() << "Window shown, starting event loop...";

    // Process events immediately for faster perceived startup
    app.processEvents();

    // Additional event processing for smoother startup
    QTimer::singleShot(0, [&app]() {
        app.processEvents();
    });

    return app.exec();
}
