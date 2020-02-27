#include "sqliterow.h"

SQLiteRow::SQLiteRow()
{

}

SQLiteRow::~SQLiteRow()
{
	size_t idx = 0, len = m_values.size();
	for(; idx < len; ++idx)
	{
		delete m_values[idx];
	}
}

void SQLiteRow::ClearValues()
{
	for(SQLiteVariant* var : m_values)
	{
		var->ClearValue();
	}
}

void SQLiteRow::SetColumnValue(const std::string& colname, const SQLiteVariant* value)
{
	*(m_valuemap[colname]) = *value;
}

void SQLiteRow::SetColumnValue(const std::string& colname, const int v)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(!var)
	{
		m_values.emplace_back(new SQLiteVariant());
		m_valuemap[colname] = m_values.back();
		var = m_values.back();
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string& colname, const long long v)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(!var)
	{
		m_values.emplace_back(new SQLiteVariant());
		m_valuemap[colname] = m_values.back();
		var = m_values.back();
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string& colname, const unsigned int v)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(!var)
	{
		m_values.emplace_back(new SQLiteVariant());
		m_valuemap[colname] = m_values.back();
		var = m_values.back();
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string& colname, const unsigned long long v)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(!var)
	{
		m_values.emplace_back(new SQLiteVariant());
		m_valuemap[colname] = m_values.back();
		var = m_values.back();
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string& colname, const float v)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(!var)
	{
		m_values.emplace_back(new SQLiteVariant());
		m_valuemap[colname] = m_values.back();
		var = m_values.back();
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string& colname, const double v)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(!var)
	{
		m_values.emplace_back(new SQLiteVariant());
		m_valuemap[colname] = m_values.back();
		var = m_values.back();
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string& colname, const std::string& v)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(!var)
	{
		m_values.emplace_back(new SQLiteVariant());
		m_valuemap[colname] = m_values.back();
		var = m_values.back();
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string& colname, const char* data, const size_t datalen)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(!var)
	{
		m_values.emplace_back(new SQLiteVariant());
		m_valuemap[colname] = m_values.back();
		var = m_values.back();
	}

	var->SetValue(data, datalen);
}

const SQLiteVariant* SQLiteRow::GetColumnValue(const std::string& colname) const
{
	return m_valuemap[colname];
}

bool SQLiteRow::GetColumnValue(const std::string& colname, int& out) const
{
	const SQLiteVariant* var = m_valuemap[colname];
	if(var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string& colname, unsigned int& out) const
{
	const SQLiteVariant* var = m_valuemap[colname];
	if(var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string& colname, long long& out) const
{
	const SQLiteVariant* var = m_valuemap[colname];
	if(var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string& colname, unsigned long long& out) const
{
	const SQLiteVariant* var = m_valuemap[colname];
	if(var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string& colname, float& out) const
{
	const SQLiteVariant* var = m_valuemap[colname];
	if(var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string& colname, double& out) const
{
	const SQLiteVariant* var = m_valuemap[colname];
	if(var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string& colname, std::string& out) const
{
	const SQLiteVariant* var = m_valuemap[colname];
	if(var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string& colname, std::vector<char>& out) const
{
	const SQLiteVariant* var = m_valuemap[colname];
	if(var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}
