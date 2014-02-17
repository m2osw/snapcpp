#pragma once

#include <QString>

class DisplayException
{
public:
    DisplayException( const QString& what, const QString& caption, const QString& message );

    void displayError();

private:
    QString f_what;
    QString f_caption;
    QString f_message;
    QString f_fullMessage;

    void genMessage();
    void outputStdError();
    void showMessageBox();
};

// vim: ts=4 sw=4 et syntax=cpp.doxygen
