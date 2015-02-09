//
// File:	googlepagerank_http.cpp
// Object:	Implementation of the HTTP slots.
//
// Copyright:	Copyright (c) 2012 Made to Order Software Corp.
//		All Rights Reserved.
//
// http://snapwebsites.org/
// contact@m2osw.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include "googlepagerank_http.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QEventLoop>

namespace googlerank
{
/** \brief Google Page Rank details namespace (i.e. private).
 *
 * This namespace includes details that are specific to the Google Page Rank
 * library. All the functions and classes defined inside that namespace are
 * private to the library and should not be used by outside programs.
 */
namespace details
{

/** \class QHttpRequest
 * \brief Object handling the network connection.
 *
 * This class connects to Google and sends an HTTP request to retrieve the
 * Google Page Rank of a given page. The URI is computed by the QGooglePageRank
 * class and provided to this object.
 *
 * To start the processing call the exec() function.
 *
 * The object runs asynchroneously, however, it does not use a separate thread
 * so you need to have a Qt event loop. If you are not using a Qt loop, the
 * process will hang and the connection is likely to get lost. You may call
 * the wait() function to wait for the result and avoid losing the connection.
 *
 * The result cannot be checked before the process finished() slot was called.
 *
 * \sa exec()
 * \sa finished()
 * \sa wait()
 */

/** \fn bool QHttpRequest::isAborted() const;
 * \brief Check whether the request was canceled in some way.
 *
 * This function returns true if the request was canceled. This happens when
 * the request returns a code other than 200 or returns an errorneous rank
 * information (data that we cannot parse properly.)
 *
 * Note that if Google returns an empty string, then it is not considered an
 * error. Instead, it is viewed as n.a. (or not indexed.)
 *
 * \note
 * You should always check whether the request was aborted before checking
 * whether it was finished because when isFinished() returns true
 * isAborted() may also return true.
 *
 * \return true if the request failed.
 *
 * \sa isFinished()
 */

/** \fn bool QHttpRequest::isFinished() const;
 * \brief Check whether the request is done.
 *
 * This function returns true once the request was fully completed. This
 * generally means the request succeeded, although the data may not be
 * correct. You should first check whether the request was aborted using
 * the isAborted() function (i.e. success is defined by isAborted().)
 *
 * \return true if the request was finished.
 *
 * \sa isAborted()
 */

/** \fn int QHttpRequest::rank() const;
 * \brief Retrieve the page rank.
 *
 * This function is called to retrieve the rank returned by Google.
 * The function raises an exception of the request is not yet finished.
 * You should not call this function if isFinished() still returns false.
 *
 * The function may return a rank (a value from 0 to 10) or an error
 * code (a negative value.) See the QGooglePageRank::QGooglePageRankStatus
 * enumeration for more details.
 *
 * \return A QGooglePageRank::QGooglePageRankStatus value.
 */

/** \fn void QHttpRequest::pageRankReady(int request, int rank);
 * \brief Signal emitted whenever the page rank is ready.
 *
 * This function is a signal. It is called whenever the function
 * finished() is called. Note that the signal is sent whether or
 * not the rank is valid. It is up to you to verify that the
 * rank corresponds to a valid number.
 *
 * The request parameter is set to the request index as indicated in
 * the QGooglePageRank object.
 *
 * \param[in] request  The request index.
 * \param[in] rank  The rank assigned to this request.
 */

/** \var QString QHttpRequest::f_uri;
 * \brief The URI to query Google with.
 *
 * This URI is the exact URI used with the network access manager to
 * request the page rank. This class does not change this URI.
 */

/** \var controlled_vars::zbool_t QHttpRequest::f_finished;
 * \brief Hold whether the request is finished.
 *
 * This variable is initialized to false. It is set to true once
 * the request ends. This does not automatically means that the
 * request succeeded.
 */

/** \var controlled_vars::zbool_t QHttpRequest::f_aborted;
 * \brief Hold whether the request was aborted.
 *
 * The request runs until it finishes or is aborted. We abort requests
 * that return more data than expected. For example, a 403 error may
 * return a lengthy message explaining that you shouldn't be tempering
 * with Google's resources. The system is likely to abort such request
 * early.
 */

/** \var QNetworkAccessManager *QHttpRequest::f_network_access_manager;
 * \brief The network access manager object.
 *
 * This variable holds a Qt network access manager. The manager is
 * allocated once needed.
 */

/** \var QNetworkReply *QHttpRequest::f_reply;
 * \brief Holds the network reply object.
 *
 * This object is used to handle the data of the network request.
 * It is a QIODevice. We read the data from this object.
 *
 * We connect to two signals from the network reply object to know
 * when data is available (readyRead) and when the request is done
 * (finished).
 *
 * \sa readyRead()
 * \sa finished()
 */

/** \var QString QHttpRequest::f_data;
 * \brief The result of the request.
 *
 * As data becomes available on the network reply, it is made
 * available in this string. This string is later parsed and
 * the rank saved in the f_rank variable.
 *
 * The data buffer may not be complete if the request gets
 * aborted (i.e. because too much data is received.)
 *
 * \sa f_reply
 * \sa f_rank
 */

/** \var QGooglePageRank::RequestType QHttpRequest::f_index;
 * \brief The index of this request.
 *
 * Whenever the QGooglePageRank object creates a QHttpRequest it
 * assigns it a number. This is that number. It starts at zero
 * and increases with each new request.
 */

/** \var QGooglePageRank::GooglePageRankStatus QHttpRequest::f_rank;
 * \brief The Google Page Rank result.
 *
 * By default this value is set to PageRankUnknown. This means the
 * request is not finished.
 *
 * Once the request says it is done, this value is set to the result
 * after we parse the f_data buffer. If f_data is empty, this value
 * is set to PageRankUndefined meaning that Google did not provide
 * a page rank (i.e. probably that the page is not even indexed by
 * Google yet.)
 *
 * Although the f_rank value is always set to a value considered valid
 * calling the rank() function fails if isFinished() is false.
 *
 * \sa f_data
 * \sa rank()
 */

/** \brief Initialize a QHttpRequest object.
 *
 * This function initializes a QHttpRequest object.
 *
 * The index is later used when the object emits the event that the page rank
 * was found.
 *
 * \param[in] parent  The parent object of this object (the QGooglePageRank)
 * \param[in] index  The index of the request using this QHttpRequest.
 */
QHttpRequest::QHttpRequest(QObject *prnt, QGooglePageRank::RequestType index)
	: QObject(prnt)
	//, f_uri("") -- auto-init
	//, f_finished(false), -- auto-init
	//, f_aborted(false), -- auto-init
	, f_network_access_manager(nullptr)
	, f_reply(nullptr)
	//, f_data(""), -- auto-init
	, f_index(index)
	, f_rank(QGooglePageRank::PageRankUnknown)
{
}

/** \brief Clean up a QHttpRequest object.
 *
 * This destructor deletes the network access manager if we allocated one.
 */
QHttpRequest::~QHttpRequest()
{
	delete f_network_access_manager;
}

/** \brief Execute the request.
 *
 * This function sends the specified URI to Google to retrieve the
 * corresponding page rank.
 *
 * The request is started asynchroneously. This means it is not done in
 * parallel, a Qt event loop is required for you to, at some point, get
 * the corresponding page rank.
 *
 * The management of the loop is based on the example shown here:
 * http://developer.qt.nokia.com/doc/qt-4.8/network-http.html
 *
 * You may call the wait() function to block until the system gets
 * the reply.
 *
 * \exception std::logic_error
 * If the URI is an empty string then the function raises this exception.
 * Also, this function can only be called once. To send multiple requests
 * created other QHttpRequests objects.
 *
 * \exception std::runtime_error
 * This exception is raised if the resulting network reply object cannot
 * properly connect to this QHttpRequest object.
 *
 * \param[in] uri  The page from which we want the Google Page Rank.
 */
void QHttpRequest::exec(const QString& uri)
{
	if(uri.isEmpty()) {
		throw std::logic_error("URI cannot be an empty string");
	}
	if(!f_uri.isEmpty() || f_network_access_manager != nullptr) {
		throw std::logic_error("QHttpRequest exec() function called twice");
	}
	f_uri = uri;
	QNetworkRequest request(f_uri);
	f_network_access_manager = new QNetworkAccessManager(this);
	f_reply = f_network_access_manager->get(request);
	if(!connect(f_reply, SIGNAL(finished()), SLOT(finished()))) {
		throw std::runtime_error("could not connect to finished() signal.");
	}
	if(!connect(f_reply, SIGNAL(readyRead()), SLOT(readyRead()))) {
		throw std::runtime_error("could not connect to readyRead() signal.");
	}
}

/** \brief Capture the finished() message from the network reply.
 *
 * This function is called as soon as the request from the network reply
 * says the system is done with the request.
 *
 * This function parses the result read by the readyRead() function and
 * saves the rank and then emits the pageRankReady signal to tell the
 * user about the result.
 *
 * Note that the signal is automatically propagated by the QGooglePageRank
 * class.
 */
void QHttpRequest::finished()
{
	f_finished = static_cast<int32_t>(true);
	extractRank();
	emit pageRankReady(f_index, f_rank);
}

/** \brief Read available data.
 *
 * This function goes to the QIODevice and reads the data that is now
 * available for the HTTP request and saves it in the f_data string
 * which will later be parsed for the rank.
 */
void QHttpRequest::readyRead()
{
	if(!f_reply) {
		throw std::runtime_error("QHttpRequest readyRead() function called with f_reply still null");
	}
	f_data += f_reply->readAll();
}

/** \brief Extract the rank from the reply.
 *
 * This function extracts the rank that the reply sent us. The
 * rank is saved in the f_rank variable so we can retrieve it
 * multiple times without having to re-extract it from the
 * string.
 *
 * When Google returns data, the string looks something like this:
 *
 * \code
 * Rank_1:1:<rank>
 * \endcode
 *
 * If the data looks invalid the rank is set to PageRankInvalid.
 * However, if the data is empty, it is not considered invalid.
 * In that case, the rank is set to PageRankUndefined which means
 * Google doesn't give that page a rank (yet.)
 *
 * \sa finished()
 * \sa rank()
 */
void QHttpRequest::extractRank()
{
	// if empty, Google says "n.a."
	if(f_data.isEmpty()) {
		f_rank = QGooglePageRank::PageRankUndefined;
		return;
	}

//qDebug() << "Request returned: [" << f_data << "]";

	// output is Rank_1:1:<rank>
	QStringList parts(f_data.split(':'));
	if(parts.count() != 3) {
		f_aborted = static_cast<int32_t>(true);
		f_rank = QGooglePageRank::PageRankInvalid;
		return;
	}

	// convert 3rd entry to integer and verify
	bool ok;
	QGooglePageRank::GooglePageRankStatus result(static_cast<QGooglePageRank::GooglePageRankStatus>(parts[2].toInt(&ok)));
	if(!ok) {
		f_aborted = static_cast<int32_t>(true);
		f_rank = QGooglePageRank::PageRankInvalid;
		return;
	}

	f_rank = result;
}

/** \brief Wait for this request to end.
 *
 * This function waits until this request is fully finished with
 * the request.
 *
 * \todo
 * We need to add a timer so the loop breaks after a fairly small
 * amount of time instead of going forever.
 */
void QHttpRequest::wait() const
{
	if(!f_finished) {
		if(!f_reply) {
			throw std::runtime_error("QHttpRequest wait() function called with f_reply still null");
		}
		// wait until finished() gets signaled
		QEventLoop loop;
		connect(f_reply, SIGNAL(finished()), &loop, SLOT(quit()));
		// can we add a QTimer somehow to not wait forever?
		loop.exec();
	}
}

} // namespace details
} // namespace googlerank
