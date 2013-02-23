/**
 * Source: http://lists.trolltech.com/qt-interest/2001-03/msg00566.html
 * Author: Jorg Preiss
 */
#include <QFile>
#include <sys/file.h>

class QLockFile: public QFile
{
public:
	/** \brief Initialize the locked file.
	 *
	 * This function initializes a default locked file.
	 */
	QLockFile()
		: QFile()
	{
	}

	/** \brief Initialize the locked file with a name.
	 *
	 * This function initializes a locked file with a filename.
	 *
	 * \param[in] name  The name of the file to open and lock.
	 */
	QLockFile(const QString& name)
		: QFile(name)
	{
	}

	/** \brief Open the locked file
	 *
	 * Open a file and lock it in share mode (if iomode is read) or
	 * exclusively (any other open mode.)
	 *
	 * The function blocks until the file is locked.
	 *
	 * When the file is closed the lock will automatically be released.
	 *
	 * \param[in] iomode  The I/O mode to use on the file.
	 *
	 * \return true if the open() succeeds, false otherwise
	 */
	virtual bool open(OpenMode iomode)
	{
		if(!QFile::open(iomode)) {
			return false;
		}
		// we want to ignore the text and unbuffered flags
		OpenMode m(iomode & ~(QIODevice::Text | QIODevice::Unbuffered));
		int op(m == QIODevice::ReadOnly ? LOCK_SH : LOCK_EX);
		// note: on close() the flock() is automatically released
  		if(flock(handle(), op) != 0) {
			QFile::close();
			return false;
		}
		// this file is now open with an exclusive lock
		return true;
	}
};

// vim: ts=4 sw=4
