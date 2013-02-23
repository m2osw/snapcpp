//
// File:	googlepagerank_http.h
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
#ifndef GOOGLEPAGERANK_HTTP_H
#define GOOGLEPAGERANK_HTTP_H

#include "googlepagerank.h"
#include <controlled_vars/controlled_vars_auto_init.h>
#include <stdexcept>

class QNetworkAccessManager;
class QNetworkReply;

namespace googlerank
{
namespace details
{



class QHttpRequest : public QObject
{
	Q_OBJECT

public:
	QHttpRequest(QObject *parent, QGooglePageRank::RequestType index);

	void exec(const QString& uri);

	bool isAborted() const
	{
		return f_aborted;
	}

	bool isFinished() const
	{
		return f_finished;
	}

	void wait() const;

	int rank() const
	{
		if(!f_finished) {
			throw std::runtime_error("rank() called before request was finished");
		}

		return f_rank;
	}

private slots:
	void finished();
	void readyRead();

signals:
	void pageRankReady(int request, int rank);

private:
	void extractRank();

	QString				f_uri;
	controlled_vars::zbool_t	f_finished;
	controlled_vars::zbool_t	f_aborted;
	QNetworkAccessManager *		f_network_access_manager;
	QNetworkReply *			f_reply;
	QString				f_data;
	QGooglePageRank::RequestType	f_index;
	QGooglePageRank::GooglePageRankStatus	f_rank;
};




} // details
} // googlerank

#endif
// #ifndef GOOGLEPAGERANK_HTTP_H
