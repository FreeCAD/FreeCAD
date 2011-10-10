/** \file
    This file and directory.h are borrowed from the dir_it library
    available at http://www.boost.org. dir_it is a directory iterator.
*/

// -*-C++-*- directory.cc
// <!!----------------------------------------------------------------------> 
// <!! Copyright (C) 1998 Dietmar Kuehl, Claas Solutions GmbH > 
// <!!> 
// <!! Permission to use, copy, modify, distribute and sell this > 
// <!! software for any purpose is hereby granted without fee, provided > 
// <!! that the above copyright notice appears in all copies and that > 
// <!! both that copyright notice and this permission notice appear in > 
// <!! supporting documentation. Dietmar Kuehl and Claas Solutions make no > 
// <!! representations about the suitability of this software for any > 
// <!! purpose. It is provided "as is" without express or implied warranty. > 
// <!!----------------------------------------------------------------------> 

// Author: Dietmar Kuehl dietmar.kuehl@claas-solutions.de 
// Title:  Implementation of the directory iterator
// Version: $Name:  $ $Id: directory.cpp,v 1.2 2006/01/30 13:23:59 wmayer Exp $

// -------------------------------------------------------------------------- 

#include "meta-iostreams.h"

#include "directory.h"

#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
#  define BOOST_UNIX 1
#elif defined(_WINDOWS) || defined(__MINGW32__) || defined (_MSC_VER)
#  define BOOST_WINNT 1
#endif 

// -------------------------------------------------------------------------- 
// The POSIX version uses the functions opendir(), readdir(), and closdir()
// to find directory entries. In addition, stat() is used to find out
// about specific file attributes.

#if defined(BOOST_UNIX)

#ifndef __USE_BSD
#define __USE_BSD
#endif 
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

struct boost::filesystem::dir_it::representation
{
	representation():
		m_handle(0),
		m_refcount(1),
		m_stat_p(false)
	{
	}

	representation(std::string const &dirname):
		m_handle(opendir(dirname.c_str())),
		m_refcount(1),
		m_directory(dirname),
		m_stat_p(false)
	{
	        if( m_directory.size() == 0 )
	                m_directory = "./" ;
		if (m_directory[m_directory.size() - 1] != '/')
			m_directory += '/';
		operator++ ();
	}

	~representation() { if ( m_handle )  closedir(m_handle); }

	representation *reference()
	{
		++m_refcount;
		return this;
	}
	representation *release() { return --m_refcount? 0: this; }

	representation &operator++()
	{
		if (m_handle)
			{
				m_stat_p = false;
				dirent *rc = readdir(m_handle);
				if (rc != 0)
					m_current = rc->d_name;
				else
					{
						m_current = "";
						closedir(m_handle);
						m_handle = 0;
					}
			}

		return *this;
	}

	bool operator== (representation const &rep) const
	{
		return (m_handle == 0) == (rep.m_handle == 0);
	}

	std::string const &operator* () { return m_current; }

	struct stat &get_stat()
	{
		if (!m_stat_p)
			stat( (m_directory + m_current).c_str(), &m_stat);
		return m_stat;
	}

	void set_mode(mode_t m, bool nv)
	{
		if (((get_stat().st_mode & m) == 0) == nv)
			chmod((m_directory + m_current).c_str(), get_stat().st_mode ^ m);
	}
	void change_owner(uid_t uid) { chown((m_directory + m_current).c_str(), uid, get_stat().st_gid); }
	void change_group(gid_t gid) { chown((m_directory + m_current).c_str(), get_stat().st_uid, gid); }

private:
	DIR         *m_handle;
	int         m_refcount;
	std::string m_directory;
	std::string m_current;
	struct stat m_stat;
	bool        m_stat_p;
};

namespace boost
{
	namespace filesystem
	{

		template <> bool get<is_link>(dir_it const &it) { return S_ISLNK(it.rep->get_stat().st_mode); }
		template <> bool get<is_regular>(dir_it const &it) { return S_ISREG(it.rep->get_stat().st_mode); }
		template <> bool get<is_directory>(dir_it const &it) { return S_ISDIR(it.rep->get_stat().st_mode); }
		template <> bool get<is_char_device>(dir_it const &it) { return S_ISCHR(it.rep->get_stat().st_mode); }
		template <> bool get<is_block_device>(dir_it const &it) { return S_ISBLK(it.rep->get_stat().st_mode); }
		template <> bool get<is_fifo>(dir_it const &it) { return S_ISFIFO(it.rep->get_stat().st_mode); }
		template <> bool get<is_socket>(dir_it const &it) { return S_ISSOCK(it.rep->get_stat().st_mode); }

		template <> bool get<user_read>(dir_it const &it) { return it.rep->get_stat().st_mode & S_IRUSR; }
		template <> void set<user_read>(dir_it const &it, bool nv) { it.rep->set_mode(S_IRUSR, nv); }
		template <> bool get<user_write>(dir_it const &it) { return it.rep->get_stat().st_mode & S_IWUSR; }
		template <> void set<user_write>(dir_it const &it, bool nv) { it.rep->set_mode(S_IWUSR, nv); }
		template <> bool get<user_execute>(dir_it const &it) { return it.rep->get_stat().st_mode & S_IXUSR; }
		template <> void set<user_execute>(dir_it const &it, bool nv) { it.rep->set_mode(S_IXUSR, nv); }
		template <> bool get<set_uid>(dir_it const &it) { return it.rep->get_stat().st_mode & S_ISUID; }
		template <> void set<set_uid>(dir_it const &it, bool nv) { it.rep->set_mode(S_ISUID, nv); }

		template <> bool get<group_read>(dir_it const &it) { return it.rep->get_stat().st_mode & S_IRGRP; }
		template <> void set<group_read>(dir_it const &it, bool nv) { it.rep->set_mode(S_IRGRP, nv); }
		template <> bool get<group_write>(dir_it const &it) { return it.rep->get_stat().st_mode & S_IWGRP; }
		template <> void set<group_write>(dir_it const &it, bool nv) { it.rep->set_mode(S_IWGRP, nv); }
		template <> bool get<group_execute>(dir_it const &it) { return it.rep->get_stat().st_mode & S_IXGRP; }
		template <> void set<group_execute>(dir_it const &it, bool nv) { it.rep->set_mode(S_IXGRP, nv); }
		template <> bool get<set_gid>(dir_it const &it) { return it.rep->get_stat().st_mode & S_ISGID; }
		template <> void set<set_gid>(dir_it const &it, bool nv) { it.rep->set_mode(S_ISGID, nv); }

		template <> bool get<other_read>(dir_it const &it) { return it.rep->get_stat().st_mode & S_IROTH; }
		template <> void set<other_read>(dir_it const &it, bool nv) { it.rep->set_mode(S_IROTH, nv); }
		template <> bool get<other_write>(dir_it const &it) { return it.rep->get_stat().st_mode & S_IWOTH; }
		template <> void set<other_write>(dir_it const &it, bool nv) { it.rep->set_mode(S_IWOTH, nv); }
		template <> bool get<other_execute>(dir_it const &it) { return it.rep->get_stat().st_mode & S_IXOTH; }
		template <> void set<other_execute>(dir_it const &it, bool nv) { it.rep->set_mode(S_IXOTH, nv); }
		template <> bool get<sticky>(dir_it const &it) { return it.rep->get_stat().st_mode & S_ISVTX; }
		template <> void set<sticky>(dir_it const &it, bool nv) { it.rep->set_mode(S_ISVTX, nv); }

		template <> nlink_t get<links>(dir_it const &it) { return it.rep->get_stat().st_nlink; }
		template <> size_t get<size>(dir_it const &it) { return it.rep->get_stat().st_size; }
		template <> unsigned long get<blocks>(dir_it const &it) { return it.rep->get_stat().st_blocks; }
		template <> unsigned long get<blksize>(dir_it const &it) { return it.rep->get_stat().st_blksize; }

		template <> mtime::value_type get<mtime>(dir_it const &it) { return &it.rep->get_stat().st_mtime; }
		template <> atime::value_type get<atime>(dir_it const &it) { return &it.rep->get_stat().st_atime; }
		template <> ctime::value_type get<ctime>(dir_it const &it) { return &it.rep->get_stat().st_ctime; }

		template <> uid_t get<uid>(dir_it const &it) { return it.rep->get_stat().st_uid; }
		template <> void set<uid>(dir_it const &it, uid_t uid) { it.rep->change_owner(uid); }
		template <> std::string get<uname>(dir_it const &it)
			{
				struct passwd *pw = getpwuid(it.rep->get_stat().st_uid);
				if (pw == 0)
					throw unknown_uid(it.rep->get_stat().st_uid);
				return pw->pw_name;
			}
		template <> void set<uname>(dir_it const &it, std::string name)
			{
				struct passwd *pw = getpwnam(name.c_str());
				if (pw != 0)
					it.rep->change_owner(pw->pw_uid);
				else
					throw unknown_uname(name);
			}

		template <> gid_t get<gid>(dir_it const &it) { return it.rep->get_stat().st_gid; }
		template <> void set<gid>(dir_it const &it, gid_t gid) { it.rep->change_group(gid); }
		template <> std::string get<gname>(dir_it const &it)
			{
				struct group *grp = getgrgid(it.rep->get_stat().st_gid);
				if (grp == 0)
					throw unknown_gid(it.rep->get_stat().st_gid);
				return grp->gr_name;
			}
		template <> void set<gname>(dir_it const &it, std::string name)
			{
				struct group *grp = getgrnam(name.c_str());
				if (grp != 0)
					it.rep->change_group(grp->gr_gid);
				else
					throw unknown_gname(name);
			}


		template <> bool get<is_hidden>(dir_it const &it) { return (*it)[0] == '.'; }
	}
}

#elif defined(BOOST_WINNT)

#include "io.h"
#include "direct.h"

struct boost::filesystem::dir_it::representation
{
	representation():
		m_handle(-1),
		m_refcount(1)
	{
	}

	representation(std::string const &dirname):
		m_handle(_findfirst((dirname + "\\*").c_str(), &m_data)),
		m_refcount(1)
	{
	}

	~representation() { if (m_handle != -1) _findclose(m_handle); }

	representation *reference()
	{
		++m_refcount;
		return this;
	}
	representation *release() { return --m_refcount? 0: this; }

	representation &operator++()
	{
		if (m_handle != -1)
			{
				if (_findnext(m_handle, &m_data) == -1)
					{
						_findclose(m_handle);
						m_handle = -1;
					}

			}

		return *this;
	}

	bool operator== (representation const &rep) const
	{
		return (m_handle == -1) == (rep.m_handle == -1);
	}

	std::string operator* () { return m_data.name; }


	struct _finddata_t const &get_data() const
	{
		return m_data;
	}

#if 0
	void set_mode(mode_t m, bool nv)
	{
		if (((get_stat().st_mode & m) == 0) == nv)
			chmod((m_directory + m_current).c_str(), get_stat().st_mode ^ m);
	}
	void change_owner(uid_t uid) { chown((m_directory + m_current).c_str(), uid, get_stat().st_gid); }
	void change_group(gid_t gid) { chown((m_directory + m_current).c_str(), get_stat().st_uid, gid); }
#endif

private:
	struct _finddata_t m_data;
	long               m_handle;
	int                m_refcount;
	std::string        m_directory;
};

namespace boost
{
	namespace filesystem
	{
#if defined(__MINGW32__)
		template <> size_t get<size>(dir_it const &it)
		{
			return it.rep->get_data().size;
		}
		template <> mtime::value_type get<mtime>(dir_it const &it)
		{
			return &it.rep->get_data().time_write;
		}
		template <> bool get<is_directory>(dir_it const &it)
		{
			return (it.rep->get_data().attrib & _A_SUBDIR) != 0;
		}
		template <> bool get<is_regular>(dir_it const &it)
		{
			return (it.rep->get_data().attrib & _A_SUBDIR) == 0;
		}
		template <> bool get<is_hidden>(dir_it const &it)
		{
			return (it.rep->get_data().attrib & _A_HIDDEN) != 0;
		}
		template <> bool get<user_read>(dir_it const &it)
		{
			return true;
		}
		template <> bool get<user_write>(dir_it const &it)
		{
			return (it.rep->get_data().attrib & _A_RDONLY) == 0;
		}
		template <> bool get<user_execute>(dir_it const &it)
		{
			std::string name(*it);
			std::string ext(name.substr(name.find_last_of('.')));
			return ext == ".exe" || ext == ".bat";
		}
#else
		get<size>::operator size::value_type() const
		{
			return m_it.rep->get_data().size;
		}
		get<mtime>::operator mtime::value_type() const
		{
			return &m_it.rep->get_data().time_write;
		}
		get<is_directory>::operator is_directory::value_type() const
		{
			return (m_it.rep->get_data().attrib & _A_SUBDIR) != 0;
		}
		get<is_regular>::operator is_regular::value_type() const
		{
			return (m_it.rep->get_data().attrib & _A_SUBDIR) == 0;
		}
		get<is_hidden>::operator is_hidden::value_type() const
		{
			return (m_it.rep->get_data().attrib & _A_HIDDEN) != 0;
		}
		get<user_read>::operator user_read::value_type() const
		{
			return true;
		}
		get<user_write>::operator user_write::value_type() const
		{
			return (m_it.rep->get_data().attrib & _A_RDONLY) == 0;
		}
		get<user_execute>::operator user_execute::value_type() const
		{
			std::string name(*m_it);
			std::string ext(name.substr(name.find_last_of('.')));
			return ext == ".exe" || ext == ".bat";
		}
#endif // __MINGW32__
	}
}
#endif

// -------------------------------------------------------------------------- 

boost::filesystem::dir_it::dir_it():
	rep(new representation())
{
}

boost::filesystem::dir_it::dir_it(std::string const &dirname):
	rep(new representation(dirname))
{
}

boost::filesystem::dir_it::dir_it(boost::filesystem::dir_it const &it):
	rep(it.rep->reference())
{
}

boost::filesystem::dir_it::~dir_it()
{
	delete rep->release();
}

boost::filesystem::dir_it &boost::filesystem::dir_it::operator= (boost::filesystem::dir_it const &it)
{
	it.rep->reference();
	delete rep->release();
	rep = it.rep;
	return *this;
}

// -------------------------------------------------------------------------- 

std::string boost::filesystem::dir_it::operator* () const
{
	return *(*rep);
}

boost::filesystem::dir_it &boost::filesystem::dir_it::operator++ ()
{
	++(*rep);
	return *this;
}

boost::filesystem::dir_it::proxy boost::filesystem::dir_it::operator++ (int)
{
	std::string rc(*(*rep));
	++(*rep);
	return rc;
}

// -------------------------------------------------------------------------- 

bool boost::filesystem::dir_it::operator== (boost::filesystem::dir_it const &it) const
{
	return *rep == *(it.rep);
}

bool boost::filesystem::dir_it::operator!= (boost::filesystem::dir_it const &it) const
{
	return !(*rep == *(it.rep));
}
