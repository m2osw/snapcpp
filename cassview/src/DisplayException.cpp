#include "DisplayException.h"

#include <iostream>

#include <QApplication>
#include <QMessageBox>

#include <snapwebsites/qstring_stream.h>

DisplayException::DisplayException( const QString& what, const QString& caption, const QString& message )
	: f_what(what)
	, f_caption(caption)
	, f_message(message)
{
    genMessage();
}


void DisplayException::genMessage()
{
    f_fullMessage = QObject::tr("Exception caught: [%1]\n%2").arg(f_what).arg(f_message);
}


void DisplayException::outputStdError()
{
	std::cerr << f_fullMessage << std::endl;
}


void DisplayException::showMessageBox()
{
	QMessageBox::critical( QApplication::activeWindow()
			, f_caption
			, f_fullMessage
			);
}


void DisplayException::displayError()
{
    outputStdError();
    showMessageBox();
}


// vim: ts=4 sw=4 et syntax=cpp.doxygen
