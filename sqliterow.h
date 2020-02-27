#ifndef SQLITEROW_H_
#define SQLITEROW_H_

#include <vector>
#include <unordered_map>

#include "sqlitevariant.h"

//Contains the values of the row
class SQLiteRow
{
	std::vector<SQLiteVariant*> m_values;
	std::unordered_map<std::string, SQLiteVariant*> m_valuemap;

public:
	SQLiteRow();
	~SQLiteRow();

	void ClearValues();

	//We can't use template member functions here because we need to provide
	//function pointers to the AngelScript engine to create bindings
	void SetColumnValue(const std::string& colname, const SQLiteVariant* value);
	void SetColumnValue(const std::string& colname, const int v);
	void SetColumnValue(const std::string& colname, const long long v);
	void SetColumnValue(const std::string& colname, const unsigned int v);
	void SetColumnValue(const std::string& colname, const unsigned long long v);
	void SetColumnValue(const std::string& colname, const float v);
	void SetColumnValue(const std::string& colname, const double v);
	void SetColumnValue(const std::string& colname, const std::string& v);
	void SetColumnValue(const std::string& colname, const char* data, const size_t datalen);

	const SQLiteVariant* GetColumnValue(const std::string& colname) const;

	bool GetColumnValue(const std::string& colname, int& out) const;
	bool GetColumnValue(const std::string& colname, unsigned int& out) const;
	bool GetColumnValue(const std::string& colname, long long& out) const;
	bool GetColumnValue(const std::string& colname, unsigned long long& out) const;
	bool GetColumnValue(const std::string& colname, float& out) const;
	bool GetColumnValue(const std::string& colname, double& out) const;
	bool GetColumnValue(const std::string& colname, std::string& out) const;
	bool GetColumnValue(const std::string& colname, std::vector<char>& out) const;
};

#endif
