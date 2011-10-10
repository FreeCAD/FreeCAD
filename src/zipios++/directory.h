/** \file
    This file and directory.cpp are borrowed from the dir_it library
    available at http://www.boost.org. dir_it is a directory iterator.
*/

// -*-C++-*- directory.h
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
// Title:  An input iterator used to list the entries in a directory 
// Version: $Name:  $ $Id: directory.h,v 1.2 2006/01/30 13:23:59 wmayer Exp $

// -------------------------------------------------------------------------- 

#if !defined(BOOST_DIRECTORY_H)
#define BOOST_DIRECTORY_H 1

// --------------------------------------------------------------------------

#include <iterator>
#include <string>
#include <cstddef>
#include <ctime>
#include <stdexcept>

// #include <boost.h>  Contents of boost.h

// Allow control over DLL version being built
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
#  define BOOST_DECL
#elif defined(ZIPIOS_DLL)
#  ifdef ZIPIOS_EXPORTS
#    define BOOST_DECL __declspec(dllexport)
#  else
#    define BOOST_DECL __declspec(dllimport)
#  endif
#else
#  define BOOST_DECL
#endif
// end of contents of boost.h

#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
#include <sys/types.h>
#endif

// --------------------------------------------------------------------------

namespace boost
{
	namespace filesystem
	{
		class dir_it;

#if defined(__GNUG__)
		template <class Property>
			typename Property::value_type get(dir_it const &);
		template <class Property>
			void set(dir_it const &, typename Property::value_type);
#else
		template <class Property> class get;
		template <class Property> class set;
#endif

	class BOOST_DECL dir_it //: public std::iterator<std::input_iterator_tag, std::string>
		{
#if defined(__GNUG__)
			template <class Property>
			friend typename Property::value_type get(dir_it const &);
			template <class Property>
			friend void set(dir_it const &, typename Property::value_type);
#endif

			struct representation;
			
		public:
			typedef ptrdiff_t   difference_type;
			typedef std::string value_type;
			typedef std::string *pointer;
			typedef std::string &reference;
			
			class proxy
			{
				friend class dir_it;
				proxy(std::string const &ent): entry(ent) {}
			public:
				std::string operator*() const { return entry; }
			private:
				std::string entry;
			};
			
			dir_it();
			dir_it(std::string const &);
			dir_it(dir_it const &);
			~dir_it();
			dir_it &operator= (dir_it const &);
			
			std::string operator* () const;
			dir_it      &operator++ ();
			proxy       operator++ (int);
			
			bool operator== (dir_it const &) const;
			bool operator!= (dir_it const &) const;

#if defined(__GNUG__)
		private:
#endif
			representation *rep;
		};
		
		struct size { typedef size_t value_type; };
		struct mtime { typedef time_t const *value_type; };

		struct is_directory { typedef bool value_type; };
		struct is_regular { typedef bool value_type; };
		struct is_hidden { typedef bool value_type; };

		struct user_read { typedef bool value_type; };
		struct user_write { typedef bool value_type; };
		struct user_execute { typedef bool value_type; };

#if defined(__GNUG__)
		template <> size::value_type get<size>(dir_it const &);
		template <> mtime::value_type get<mtime>(dir_it const &);
		template <> bool get<is_directory>(dir_it const &);
		template <> bool get<is_regular>(dir_it const &);
		template <> bool get<is_hidden>(dir_it const &);
		template <> bool get<user_read>(dir_it const &);
		template <> void set<user_read>(dir_it const &, bool);
		template <> bool get<user_write>(dir_it const &);
		template <> void set<user_write>(dir_it const &, bool);
		template <> bool get<user_execute>(dir_it const &);
		template <> void set<user_execute>(dir_it const &, bool);
#else
		template <> class BOOST_DECL get<size>
		{
			typedef size::value_type value_type;
		public:
			get(dir_it const &it): m_it(it) {}
			operator value_type() const;
		private:
			dir_it const &m_it;
		};

		template <> class BOOST_DECL get<mtime>
		{
			typedef mtime::value_type value_type;
		public:
			get(dir_it const &it): m_it(it) {}
			operator value_type() const;
		private:
			dir_it const &m_it;
		};

		template <> class BOOST_DECL get<is_directory>
		{
			typedef is_directory::value_type value_type;
		public:
			get(dir_it const &it): m_it(it) {}
			operator value_type() const;
		private:
			dir_it const &m_it;
		};

		template <> class BOOST_DECL get<is_regular>
		{
			typedef is_regular::value_type value_type;
		public:
			get(dir_it const &it): m_it(it) {}
			operator value_type() const;
		private:
			dir_it const &m_it;
		};

		template <> class BOOST_DECL get<is_hidden>
		{
			typedef is_hidden::value_type value_type;
		public:
			get(dir_it const &it): m_it(it) {}
			operator value_type() const;
		private:
			dir_it const &m_it;
		};
		template <> class BOOST_DECL set<is_hidden>
		{
		public:
			set(dir_it const &, is_hidden::value_type);
		};

		template <> class BOOST_DECL get<user_read>
		{
			typedef user_read::value_type value_type;
		public:
			get(dir_it const &it): m_it(it) {}
			operator value_type() const;
		private:
			dir_it const &m_it;
		};

		template <> class BOOST_DECL get<user_write>
		{
			typedef user_write::value_type value_type;
		public:
			get(dir_it const &it): m_it(it) {}
			operator value_type() const;
		private:
			dir_it const &m_it;
		};
		template <> class BOOST_DECL set<user_write>
		{
		public:
			set(dir_it const &, user_write::value_type);
		};

		template <> class BOOST_DECL get<user_execute>
		{
			typedef user_execute::value_type value_type;
		public:
			get(dir_it const &it): m_it(it) {}
			operator value_type() const;
		private:
			dir_it const &m_it;
		};

#endif

#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)

		struct is_link { typedef bool value_type; };
		template <> bool get<is_link>(dir_it const &);

		struct is_char_device { typedef bool value_type; };
		template <> bool get<is_char_device>(dir_it const &);

		struct is_block_device { typedef bool value_type; };
		template <> bool get<is_block_device>(dir_it const &);

		struct is_fifo { typedef bool value_type; };
		template <> bool get<is_fifo>(dir_it const &);

		struct is_socket { typedef bool value_type; };
		template <> bool get<is_socket>(dir_it const &);

		struct atime { typedef time_t *value_type; };
		template <> atime::value_type get<atime>(dir_it const &);
		struct ctime { typedef time_t *value_type; };
		template <> ctime::value_type get<ctime>(dir_it const &);

		struct group_read { typedef bool value_type; };
		template <> bool get<group_read>(dir_it const &);
		template <> void set<group_read>(dir_it const &, bool);
		struct group_write { typedef bool value_type; };
		template <> bool get<group_write>(dir_it const &);
		template <> void set<group_write>(dir_it const &, bool);
		struct group_execute { typedef bool value_type; };
		template <> bool get<group_execute>(dir_it const &);
		template <> void set<group_execute>(dir_it const &, bool);
		struct other_read { typedef bool value_type; };
		template <> bool get<other_read>(dir_it const &);
		template <> void set<other_read>(dir_it const &, bool);
		struct other_write { typedef bool value_type; };
		template <> bool get<other_write>(dir_it const &);
		template <> void set<other_write>(dir_it const &, bool);
		struct other_execute { typedef bool value_type; };
		template <> bool get<other_execute>(dir_it const &);
		template <> void set<other_execute>(dir_it const &, bool);

		struct set_uid { typedef bool value_type; };
		template <> bool get<set_uid>(dir_it const &);
		template <> void set<set_uid>(dir_it const &, bool);
		struct set_gid { typedef bool value_type; };
		template <> bool get<set_gid>(dir_it const &);
		template <> void set<set_gid>(dir_it const &, bool);
		struct sticky { typedef bool value_type; };
		template <> bool get<sticky>(dir_it const &);
		template <> void set<sticky>(dir_it const &, bool);

		struct mode { typedef mode_t value_type; };
		template <> mode_t get<mode>(dir_it const &);
		template <> void set<mode>(dir_it const &, mode_t);

		struct links { typedef nlink_t value_type; };
		template<> nlink_t get<links>(dir_it const &);
		struct blocks { typedef unsigned long value_type; };
		template<> unsigned long get<blocks>(dir_it const &);
		struct blksize { typedef unsigned long value_type; };
		template<> unsigned long get<blksize>(dir_it const &);

		class unknown_uid: public std::invalid_argument
		{
		public:
			unknown_uid(uid_t u): std::invalid_argument("unknown user ID"), m_uid(u) {}
			virtual ~unknown_uid() throw() {}
			uid_t uid() const { return m_uid; }
		private:
			uid_t m_uid;
		};
		struct uid { typedef uid_t value_type; };
		template<> uid_t get<uid>(dir_it const &);
		template<> void set<uid>(dir_it const &, uid_t);
		class unknown_uname: public std::invalid_argument
		{
		public:
			unknown_uname(std::string u): std::invalid_argument("unknown user name"), m_uname(u) {}
			virtual ~unknown_uname() throw() {}
			std::string uname() const { return m_uname; }
		private:
			std::string m_uname;
		};
		struct uname { typedef std::string value_type; };
		template<> std::string get<uname>(dir_it const &);
		template<> void set<uname>(dir_it const &, std::string );

		class unknown_gid: public std::invalid_argument
		{
		public:
			unknown_gid(gid_t g): std::invalid_argument("unknown group ID"), m_gid(g) {}
			virtual ~unknown_gid() throw() {}
			gid_t gid() const { return m_gid; }
		private:
			gid_t m_gid;
		};
		struct gid { typedef gid_t value_type; };
		template<> gid_t get<gid>(dir_it const &);
		template<> void set<gid>(dir_it const &, gid_t);
		class unknown_gname: public std::invalid_argument
		{
		public:
			unknown_gname(std::string g): std::invalid_argument("unknown group name"), m_gname(g) {}
			virtual ~unknown_gname() throw() {}
			std::string gname() const { return m_gname; }
		private:
			std::string m_gname;
		};
		struct gname { typedef std::string value_type; };
		template<> std::string get<gname>(dir_it const &);
		template<> void set<gname>(dir_it const &, std::string );

#endif

	} // namespace filesystem
} // namespace boost

namespace std
{
	template<>
	struct iterator_traits<boost::filesystem::dir_it>
	{
	public:
		typedef ptrdiff_t          difference_type;
		typedef std::string             value_type;
		typedef std::string             *pointer;
		typedef std::string             &reference;
		typedef input_iterator_tag iterator_category;
	};
} // namespace std

// --------------------------------------------------------------------------

#endif /* BOOST_DIRECTORY_H */
