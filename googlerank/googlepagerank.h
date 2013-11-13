//
// File:	googlepagerank.h
// Object:	Allows us to gather the rank of a page as defined by Google.
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
#ifndef GOOGLEPAGERANK_H
#define GOOGLEPAGERANK_H
#include <QObject>
#include <QUrl>

namespace googlerank
{

class QGooglePageRank : public QObject
{
	Q_OBJECT

public:
	enum GooglePageRankStatus {
		PageRankError = -4, // the request failed in some way
		PageRankInvalid = -3, // the request did not return a valid rank
		PageRankUnknown = -2, // the request did not complete yet

		PageRankUndefined = -1, // the request failed (n.a.)
		PageRank0 = 0,
		PageRank1,
		PageRank2,
		PageRank3,
		PageRank4,
		PageRank5,
		PageRank6,
		PageRank7,
		PageRank8,
		PageRank9,
		PageRank10
	};

	typedef int RequestType;

	QGooglePageRank(QObject *prnt = NULL) : QObject(prnt) {}
	virtual ~QGooglePageRank() {}

	static QString urlHash(const QUrl& url);
	RequestType requestRank(const QUrl& url, bool test = false);
	GooglePageRankStatus pageRank(RequestType request, bool wait = false);

signals:
	void googlePageRank(int request, int rank);

private slots:
	void pageRankReady(int request, int rank);
};


} // googlerank
#endif
// #ifndef GOOGLEPAGERANK_H
