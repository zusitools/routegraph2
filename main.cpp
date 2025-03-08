#include "mainwindow.h"
#include "routegraphapplication.h"

#include <QDebug>
#include <QString>
#include <QTextStream>
#include <QStandardPaths>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>

#ifdef WIN32
#define BOOST_STACKTRACE_USE_WINDBG
#endif

#include <boost/preprocessor/stringize.hpp>
#include <boost/stacktrace.hpp>

#ifdef WIN32
#include <windows.h>
#else
#include <signal.h>
#endif

#include <fstream>
#include <filesystem>

std::string g_stacktraceFilename;

#ifdef WIN32
LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS*) {
#else
void SignalHandler(int signum) {
    ::signal(signum, SIG_DFL);
#endif
    boost::stacktrace::safe_dump_to(0, 20, g_stacktraceFilename.data());
#ifdef WIN32
    return EXCEPTION_EXECUTE_HANDLER;
#else
    ::raise(SIGABRT);
#endif
}

int main(int argc, char *argv[]) {
    RoutegraphApplication a(argc, argv);

    g_stacktraceFilename = (QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0) + "/routegraph2_stacktrace.dmp").toStdString();

#ifdef WIN32
    ::SetUnhandledExceptionFilter(ExceptionHandler);
#else
    ::signal(SIGSEGV, &SignalHandler);
    ::signal(SIGABRT, &SignalHandler);
#endif

    if (std::filesystem::exists(g_stacktraceFilename)) {
        std::ifstream ifs(g_stacktraceFilename);
        const auto stacktrace = boost::stacktrace::stacktrace::from_dump(ifs);
        ifs.close();

        std::ostringstream os;
        os << "Beim letzten Mal ist das Programm abgestürzt.\nBitte Strg+C drücken, um den Inhalt dieses Fensters zu kopieren,"
           << " und diesen im Zusi-Forum einstellen.\n\nProgrammversion: " BOOST_PP_STRINGIZE(ROUTEGRAPH2_VERSION) "\n\n";
        os << stacktrace;

        QMessageBox messageBox;
        messageBox.setText(QString::fromStdString(os.str()));
        auto* deleteButton = messageBox.addButton("Absturzbericht löschen", QMessageBox::DestructiveRole);
        messageBox.setDefaultButton(deleteButton);
        messageBox.addButton("Absturzbericht beim nächsten Start nochmals anzeigen", QMessageBox::AcceptRole);
        messageBox.exec();
        if (messageBox.clickedButton() == deleteButton) {
	    std::filesystem::remove(g_stacktraceFilename.data());
        }
    }

    MainWindow w;
    w.show();

    return a.exec();
}
