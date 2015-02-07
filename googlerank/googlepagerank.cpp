//
// File:	googlepagerank.cpp
// Object:	Generates the Google Page Rank library
//
// The computation of the hash is based on code found on:
// https://github.com/phurix/pagerank/
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
#include "googlepagerank.h"
#include "googlepagerank_http.h"
#include <stdexcept>



/** \mainpage Google Page Rank
 *
 * This documentation gives details about the Google Page Rank API. The
 * project includes a library that one can use to query Google servers for
 * the rank of a page available on the Internet.
 *
 * The project also includes a command line tool called pagerank that can
 * be used to query the rank of a page directly on your command line:
 *
 * \code
 *   pagerank [-t] [-r] 'http://www.example.com/'
 * \endcode
 *
 * The library makes use of Qt and the Qt network environment. This means
 * for you to use it you need to make your application a Qt application
 * (i.e. use QApplication or QCoreApplication).
 *
 * The request is sent and works asynchroneously, which gives you a way
 * to request multiple pages without having to wait for a previous request
 * to first return. This can be a problem since Google is likely to ban your
 * IP address if it receives too many requests in a row. However, you are
 * responsible for timing your requests to the Google robots.
 *
 * The use of the API can be as simple as the following (which does not use
 * the asynchroneous capabilities):
 *
 * \code
 * QGooglePageRank pr;
 * QString url("http://www.example.com/");
 * QGooglePageRank::RequestType req = pr.requestRank(url);
 * QGooglePageRank::GooglePageRankStatus rank = pr.pageRank(req, true);
 * \endcode
 *
 * Note that in this case we ask the pageRank() function to wait until the
 * request returns. This means the function may block for a minute. This
 * example comes from the pagerank.cpp file.
 *
 * The QGooglePageRank also offers a signal which is emitted each time
 * a request returns. That event includes the request index and the rank
 * of the page that was requested. Obviously, the signal will happen properly
 * only if you properly use the Qt environment and have a Qt loop somewhere.
 *
 * \code
 * ...
 *   pr = new QGooglePageRank(this);
 *   connect(pr, SIGNAL("googlePageRank(int, int)"),
 *           this, SLOT("gotPageRank(int, int)"));
 *   RequestType req = pr->requestRank("http://www.example.com/some-page.html");
 * ...
 * void gotPageRank(int request, int rank)
 * {
 *   // do something with the rank
 *   // request here == req from requestRank() call
 * }
 * \endcode
 *
 * The message transmissions go roughly like this:
 *
 * \msc
 * You,QGooglePageRank,QHttpRequest,QNetworkReply,Google;
 * You=>QGooglePageRank [label="requestRank()"];
 * QGooglePageRank=>QHttpRequest [label="exec()"];
 * QHttpRequest=>QNetworkReply [label="URI"];
 * QNetworkReply->Google [label="[HTTP/1.1]"];
 * QHttpRequest<<QNetworkReply [label="return"];
 * QGooglePageRank<<QHttpRequest [label="return"];
 * You<<QGooglePageRank [label="return"];
 * ...;
 * Google->QNetworkReply [label="[HTTP/1.1 reply]"];
 * QNetworkReply->QHttpRequest [label="finished()"];
 * QHttpRequest->QGooglePageRank [label="pageRankReady()"];
 * QGooglePageRank->You [label="googlePageRank()"];
 * \endmsc
 *
 * If you do not connect() to the googlePageRank(), then you will have to
 * call the pageRank() function. In that case, it checks whether the rank
 * was defined. If not it returns immediately with PageRankUnknown.
 * This is represented as a very simple call:
 *
 * \msc
 * You,QGooglePageRank,QHttpRequest;
 * You=>QGooglePageRank [label="pageRank()"];
 * QGooglePageRank=>QHttpRequest [label="rank()"];
 * QGooglePageRank<<QHttpRequest [label="return"];
 * You<<QGooglePageRank [label="return"];
 * \endmsc
 *
 * Here we do not show all the checks of the pageRank() function before
 * calling the rank() function. The pageRank() function has to make sure
 * that the request was completed before attempting to get the rank.
 * Until then, the function returns an error.
 *
 * At this point there is no proper handling of timeouts. I assume that
 * the Qt network objects timeout on their own.
 *
 * \sa googlerank::QGooglePageRank
 * \sa googlerank::QGooglePageRank::requestRank()
 * \sa googlerank::QGooglePageRank::pageRank()
 * \sa googlerank::details::QHttpRequest::rank()
 * \sa main()
 */



/** \brief Google Page Rank library namespace.
 *
 * This namespace includes the Google Page Rank classes and functions.
 */
namespace googlerank
{

/** \class QGooglePageRank
 * \brief Class used to create connections to Google asking for page ranks.
 *
 * This class is used to request page rank information from Google.
 * Use the requestRank() function to initiate a request and then
 * pageRank() to retrieve the result, or, if you prefer, use the
 * googlePageRank() signal.
 *
 * The request works asynchroneously so it loads the signal information
 * while you do other work. The system works only if you call a
 * Qt event loop so the network system is given a chance to run. Not
 * calling any event loop may prevent the system from receiving the
 * information and never connect or lose the network connection.
 *
 * The same QGooglePageRank object can be used to query multiple
 * pages: (1) be very careful as Google may block your IP address if
 * you request too many pages at once; and (2) all the requests allocate
 * memory and keeps it allocated as long as the QGooglePageRank remains
 * allocated; deleting the QGooglePageRank will delete all the buffers
 * allocated for the request. This is especially important if you plan
 * to retrieve many pages, you probably want to create a new
 * QGooglePageRank each time to make sure you don't grow your process
 * memory usage.
 *
 * \sa requestRank()
 * \sa pageRank()
 * \sa googlePageRank()
 */

/** \fn QGooglePageRank::QGooglePageRank(QObject *parent);
 * \brief Initialize the QGooglePageRank object.
 *
 * The constructor initializes the Google Page Rank object. The result is
 * a default setup but no requests. To initiate a request use the
 * requestRank() function.
 *
 * It is done this way so you have the opportunity to connect to the
 * googlePageRank() signal before you start processing a request.
 *
 * Note that this is a QObject, not a QWidget. To display the page rank
 * in a Qt window, use a QLabel or some other widget.
 *
 * \param[in] parent  The parent of the QGooglePageRank object.
 *
 * \sa googlePageRank()
 */

/** \fn virtual QGooglePageRank::~QGooglePageRank();
 * \brief Destroys a Google Page Rank object.
 *
 * This function cleans up a Google Page Rank object as required. This means
 * deleting all the requests that were sent to the object.
 */

/** \fn void QGooglePageRank::googlePageRank(int request, int rank);
 * \brief Signal sent when the page rank is available.
 *
 * When you create a QGooglePageRank object and call requestRank(), the
 * result is to be sent to all the connected object via the googlePageRank()
 * signal. One way to get the page rank of a page is to use the signal
 * as in:
 *
 * \code
 * ...
 *   pr = new QGooglePageRank(this);
 *   connect(pr, SIGNAL("googlePageRank(int, int)"),
 *           this, SLOT("gotPageRank(int, int)"));
 *   RequestType req = pr->requestRank("http://www.example.com/some-page.html");
 * ...
 * void gotPageRank(int request, int rank)
 * {
 *   // do something with the rank
 *   // request here == req from requestRank() call
 * }
 * \endcode
 *
 * This is similar to getting the rank with the pageRank() function, but it will
 * be called as soon as it is available and you do not need to block while
 * waiting for the information.
 *
 * Note that you can call requestRank() any number of times. However, it will:
 *
 * \li (1) Add a new set of network classes that do not get released until you
 *         delete the QGooglePageRank object;
 * \li (2) Process all the requests in parallel, meaning that Google is likely
 *         to get mad at you and block you IP.
 *
 * Remember that Google does not allow robots from using their services without
 * an agreement. This library is expected to be used only as a side effect of a
 * user action (i.e. a user is visiting the page from which the page rank
 * is being requested.)
 */

/** \typedef int QGooglePageRank::RequestType;
 * \brief The type of a request identifier.
 *
 * This type is used as the request identifier. These identifiers are allocated
 * at the time you create a request with the requestRank(). If you use only
 * one request with each QGooglePageRank object, then the identifier will always
 * be zero.
 */

/** \enum QGooglePageRank::GooglePageRankStatus;
 * \brief Enumeration of all the ranks returned by the QGooglePageRank class.
 *
 * The values defined in the GooglePageRankStatus enumeration are all
 * the possible values returned by the QGooglePageRank class. Each is defined
 * below.
 *
 * No other value will legally be returned by this class.
 *
 * The PageRankUndefined is viewed as a legal Google rank. It is used when
 * the rank is undefined (often represented as n.a.).
 *
 * Other negative values are viewed as errors.
 */

/** \var QGooglePageRank::PageRankError
 * \brief An error occured.
 *
 * This rank represents the fact that an error occured while requesting the
 * rank from Google.
 */

/** \var QGooglePageRank::PageRankInvalid
 * \brief The page data is not valid.
 *
 * This error is returned when the data read from Google is not valid rank
 * information. This can happen if you get banned or the hash becomes
 * invalid and Google starts return a 403 error.
 */

/** \var QGooglePageRank::PageRankUnknown
 * \brief The rank is not yet known.
 *
 * This error is used between the time the request is sent and the finished()
 * event occurs. This represents the fact that we do not yet know the response.
 */

/** \var QGooglePageRank::PageRankUndefined
 * \brief The rank is not defined.
 *
 * This enumeration is considered a valid rank value. It represents the case
 * when Google replies positively (i.e. without a 403 or some other error) but
 * does not rank the specified page.
 */

/** \var QGooglePageRank::PageRank0
 * \brief This page is indexed.
 *
 * This is the lowest rank representing that the page is indexed but it has no
 * real authority.
 */

/** \var QGooglePageRank::PageRank1
 * \brief This has a Google Rank of 1.
 *
 * This value represents the Google Rank of 1.
 */

/** \var QGooglePageRank::PageRank2,
 * \brief This has a Google Rank of 2.
 *
 * This value represents the Google Rank of 2.
 */

/** \var QGooglePageRank::PageRank3,
 * \brief This has a Google Rank of 3.
 *
 * This value represents the Google Rank of 3.
 */

/** \var QGooglePageRank::PageRank4,
 * \brief This has a Google Rank of 4.
 *
 * This value represents the Google Rank of 4.
 */

/** \var QGooglePageRank::PageRank5,
 * \brief This has a Google Rank of 5.
 *
 * This value represents the Google Rank of 5.
 */

/** \var QGooglePageRank::PageRank6,
 * \brief This has a Google Rank of 6.
 *
 * This value represents the Google Rank of 6.
 */

/** \var QGooglePageRank::PageRank7,
 * \brief This has a Google Rank of 7.
 *
 * This value represents the Google Rank of 7.
 */

/** \var QGooglePageRank::PageRank8,
 * \brief This has a Google Rank of 8.
 *
 * This value represents the Google Rank of 8.
 */

/** \var QGooglePageRank::PageRank9,
 * \brief This has a Google Rank of 9.
 *
 * This value represents the Google Rank of 9.
 */

/** \var QGooglePageRank::PageRank10
 * \brief This has a Google Rank of 10.
 *
 * This value represents the Google Rank of 10.
 */

/** \brief Compute a Google Page Rank Hash version 8.
 *
 * This function computes the page rank hash necessary to query
 * Google for the rank of the page as specified by the \p url
 * parameter.
 *
 * The result of the function is a string that can be used as the
 * ch query parameter (...&ch=\<result>&...)
 *
 * In general, you do not need to call this function as the requestRank()
 * function does it for you as required.
 *
 * The code found in urlHash() comes from an algorithm found here:
 * http://pagerank.phurix.net/
 *
 * This algorithm is used in a Firefox toolbar.
 *
 * \note
 * The URL to be transformed includes the protocol. However, contrary
 * to some older hashing versions, it does not include the "info:" part
 * later added to the request to Google.
 * 
 * \param[in] url   The URL to transform in a hash.
 *
 * \return The string representing the hash number starting with an 8.
 */
QString QGooglePageRank::urlHash(const QUrl& url)
{
#define SEED	"Mining PageRank is AGAINST GOOGLE'S TERMS OF SERVICE. Yes, I'm talking to you, scammer."
	static const char *seed = SEED;
	static const int seedlen(sizeof(SEED) - 1); // -1 to ignore the '\0'

	unsigned int key(16909125);

	QString uri(url.toString());
	int uri_len(uri.length());
	for(int i = 0; i < uri_len; ++i)
	{
		unsigned char c(uri.at(i).unicode());
		key ^= seed[i % seedlen] ^ static_cast<unsigned char>(c);
		key = key >> 23 | key << 9;
	}

	return QString("8%1").arg(key, 0, 16);
}

/** \brief Start a Google Page Rank request.
 *
 * This function initializes a page rank request and starts processing the
 * request asynchroneously.
 *
 * Because we use the Qt library, it is necessary for your application to
 * either use QApplication or QCoreApplication or this request will fail.
 *
 * The return value must be saved to later call the pageRank() function
 * and retrieve the actual rank of the page.
 *
 * \param[in] url  The URL to query from Google.
 * \param[in] test  Whether the request is sent to Google (false) or our
 *                  servers (true).
 *
 * \return The request index.
 */
QGooglePageRank::RequestType QGooglePageRank::requestRank(const QUrl& url, bool test)
{
	// Dynamic parameters:
	// "...&ch=<checksum>&q=info:<uri>";

	QString query;

	if(test) {
		query = QString("http://alexis.m2osw.com/pagerank.php?client=navclient-auto&features=Rank&ch=%1&q=info:%2")
				.arg(urlHash(url))
				.arg(QString(url.toEncoded()));
	}
	else {
		query = QString("http://toolbarqueries.google.com/tbr?client=navclient-auto&features=Rank&ch=%1&q=info:%2")
			.arg(urlHash(url))
			.arg(QString(url.toEncoded()));
	}

	RequestType result(children().size());
	details::QHttpRequest *req = new details::QHttpRequest(this, result);
	connect(req, SIGNAL(pageRankReady(int, int)), this, SLOT(pageRankReady(int, int)));
	req->exec(query);

	return result;
}

/** \brief Retrieve the page rank.
 *
 * This function is used to retrieve the rank of a page which request
 * was previously started using the requestRank() function.
 *
 * By default the function returns immediately. If the result is not
 * yet available, then PageRankUnknown is returned.
 *
 * The wait parameter can be set to true to wait for the request to
 * finish before the function returns. This can take a long time.
 *
 * If the request gets aborted (too long, error instead of 200, etc.)
 * then this function returns the PageRankInvalid error.
 *
 * \warning
 * Keep in mind that sending too many requests to at once to Google
 * will result in your IP address getting blocked. This means using
 * the wait parameter and setting to true is generally a good idea
 * to avoid sending more than one query at a time.
 *
 * \return The page rank or one of the page rank errors.
 */
QGooglePageRank::GooglePageRankStatus QGooglePageRank::pageRank(RequestType request, bool wait)
{
	if(request < 0 || request >= children().size()) {
		throw std::range_error("request number too small or too large");
	}

	details::QHttpRequest *req = dynamic_cast<details::QHttpRequest *>(children()[request]);
	if(req == nullptr) {
		throw std::logic_error("somehow the QHttpRequest pointer could not be converted to a QHttpRequest");
	}
	if(req->isAborted()) {
		return PageRankInvalid;
	}
	if(!req->isFinished()) {
		if(!wait) {
			return PageRankUnknown;
		}
		req->wait();
		if(req->isAborted()) {
			return PageRankInvalid;
		}
	}

	// we've got the reply
	return static_cast<GooglePageRankStatus>(req->rank());
}

/** \brief Slot implementation, re-emits the signal.
 *
 * This function re-emits the signal from the child in the
 * parent. This way outsiders can connect to this signal.
 *
 * We don't do anything with the signal as the rest of the
 * object makes use of the rank(), isFinished(), and
 * isAborted() functions instead.
 *
 * The types in the message use int instead of the typedef
 * and enum types because the Qt signals are limited in this
 * way.
 *
 * \param[in] request  The request index (RequestType)
 * \param[in] rank  The rank (GooglePageRankStatus)
 */
void QGooglePageRank::pageRankReady(int request, int rank)
{
	emit googlePageRank(request, rank);
}

} // googlerank
