#pragma once

#include <map>
#include <string>

#include "exception.hpp"

template <typename T>
class map
{
private:
	typedef std::map<std::string,T> imap;
	typedef std::pair<std::string,T> ipair;
	imap _map;
	
public:
	map() {}
	~map() {}
	
	typename imap::iterator begin()
	{
		return _map.begin();
	}
	
	typename imap::const_iterator begin() const
	{
		return _map.cbegin();
	}
	
	typename imap::iterator end()
	{
		return _map.end();
	}
	
	typename imap::const_iterator end() const
	{
		return _map.cend();
	}
	
	void insert(const std::string &key, T elem)
	{
		_map.insert(ipair(key,elem));
	}
	
	void insert(const ipair &pair)
	{
		_map.insert(pair);
	}
	
	typename imap::iterator find(const std::string &key)
	{
		return _map.find(key);
	}
	
	typename imap::const_iterator find(const std::string &key) const
	{
		return _map.find(key);
	}
	
	void remove(const std::string &key)
	{
		_map.erase(_map.find(key));
	}
	
	T operator [](const std::string &key) throw(exception)
	{
		auto iter = find(key);
		if(iter == end())
		{
			throw exception((std::string("there is no object '") + key + std::string("' in map")).data());
		}
		return iter->second;
	}
	
	const T operator [](const std::string &key) const throw(exception)
	{
		auto iter = find(key);
		if(iter == end())
		{
			throw exception(std::string("there is no object '") + key + std::string("' in map"));
		}
		return iter->second;
	}
};
