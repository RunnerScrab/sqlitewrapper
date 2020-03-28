#include "sqlitetable.h"

#include "utils.h"

#ifndef TESTING_
#include "as_addons/scriptarray.h"
#include "angelscript.h"
#include "database.h"
#endif

#include "sqliterow.h"
#include "sqliteutil.h"
#include "sqlitevariant.h"

SQLiteColumn::SQLiteColumn(const std::string& name, SQLiteVariant::StoredType vartype,
			KeyType keytype, SQLiteTable* foreigntable, const std::string& foreignname)
{
	m_name = name;
	m_coltype = vartype;
	m_keytype = keytype;
	m_foreigntable = foreigntable;
	m_foreignkeyname = foreignname;
#ifndef TESTING_
	if(foreigntable)
	{
		foreigntable->AddRef();
	}
#endif
}

SQLiteColumn::~SQLiteColumn()
{
#ifndef TESTING_
	if(m_foreigntable)
	{
		m_foreigntable->Release();
	}
#endif
}

SQLiteColumn::SQLiteColumn(SQLiteColumn&& other)
{
	m_coltype = std::move(other.m_coltype);
	m_name = std::move(other.m_name);
	m_foreignkeyname = std::move(other.m_foreignkeyname);
	m_keytype = std::move(other.m_keytype);
	m_foreigntable = std::move(other.m_foreigntable);
}

sqlite3* SQLiteTable::m_static_pDB = 0;

void SQLiteTable::SetDBConnection(sqlite3* pDB)
{
	m_static_pDB = pDB;
}

sqlite3* SQLiteTable::GetDBConnection()
{
	return m_static_pDB;
}

#ifndef TESTING_
int RegisterDBTable(sqlite3* sqldb, asIScriptEngine* sengine)
{
	int result = 0;
	result = sengine->RegisterObjectType("DBTable", 0, asOBJ_REF);

	RETURNFAIL_IF(result < 0);
	result = sengine->RegisterObjectBehaviour("DBTable", asBEHAVE_FACTORY, "DBTable@ f(string& in)",
						asFUNCTION(SQLiteTable::Factory), asCALL_CDECL);
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterObjectBehaviour("DBTable", asBEHAVE_ADDREF, "void f()", asMETHOD(SQLiteTable, AddRef),
						asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterObjectBehaviour("DBTable", asBEHAVE_RELEASE, "void f()", asMETHOD(SQLiteTable, Release),
						asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterObjectMethod("DBTable", "DBRow@ MakeRow()",
					asMETHODPR(SQLiteTable, CreateRow, (void), SQLiteRow*), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterEnum("DBColKeyType");
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterEnumValue("DBColKeyType", "DBKEYTYPE_NOTKEY",
					SQLiteColumn::KeyType::KEY_NONE);
	RETURNFAIL_IF(result < 0);
	result = sengine->RegisterEnumValue("DBColKeyType", "DBKEYTYPE_PRIMARY",
					SQLiteColumn::KeyType::KEY_PRIMARY);
	RETURNFAIL_IF(result < 0);
	result = sengine->RegisterEnumValue("DBColKeyType",
					"DBKEYTYPE_AUTO",
					SQLiteColumn::KeyType::KEY_AUTO_PRIMARY);
	RETURNFAIL_IF(result < 0);
	result = sengine->RegisterEnumValue("DBColKeyType", "DBKEYTYPE_FOREIGN",
					SQLiteColumn::KeyType::KEY_FOREIGN);
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterObjectMethod("DBTable", "string GetName() const",
					asMETHODPR(SQLiteTable, GetName, (void) const, std::string), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterObjectMethod("DBTable",
					"void AddIntCol(const string& in, DBColKeyType keytype"
					" = DBKEYTYPE_NOTKEY, DBTable@ foreign_table = null)",
					asMETHODPR(SQLiteTable, AddIntColumn,
						(const std::string&, SQLiteColumn::KeyType, SQLiteTable*), bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterObjectMethod("DBTable",
					"void AddRealCol(const string& in, DBColKeyType keytype"
					" = DBKEYTYPE_NOTKEY, DBTable@ foreign_table = null)",
					asMETHODPR(SQLiteTable, AddRealColumn,
						(const std::string&, SQLiteColumn::KeyType, SQLiteTable*), bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterObjectMethod("DBTable",
					"void AddTextCol(const string& in, DBColKeyType keytype"
					" = DBKEYTYPE_NOTKEY, DBTable@ foreign_table = null)",
					asMETHODPR(SQLiteTable, AddTextColumn,
						(const std::string&, SQLiteColumn::KeyType, SQLiteTable*), bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterObjectMethod("DBTable",
					"void AddUUIDCol(const string& in, DBColKeyType keytype"
					" = DBKEYTYPE_NOTKEY, DBTable@ foreign_table = null)",
					asMETHODPR(SQLiteTable, AddBlobColumn,
						(const std::string&, SQLiteColumn::KeyType, SQLiteTable*), bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterObjectMethod("DBTable", "DBTable@ CreateSubTable(const string& in)",
					asMETHODPR(SQLiteTable, CreateSubTable, (const std::string&), SQLiteTable*),
					asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterObjectMethod("DBTable", "bool LoadSubTable(DBRow@ parent_row, array<DBRow@>@ results)",
					asMETHODPR(SQLiteTable, LoadSubTable, (SQLiteRow*, CScriptArray*), bool),
					asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);
	return result;
}
#endif

SQLiteTable* SQLiteTable::CreateSubTable(const std::string& name)
{
	//Called from the parent - used to create a table to store an array

	std::string subtablename = GetName() + "_" + name;
	SQLiteTable* subtable = new SQLiteTable(m_pDB, subtablename.c_str());

	m_subtablemap[subtablename] = subtable;
	subtable->m_bIsSubTable = true;
#ifndef TESTING_
	subtable->AddRef();
#endif

	for(size_t idx = 0, len = m_primary_keycols.size();
	    idx < len; ++idx)
	{
		SQLiteColumn* pkeycol = m_primary_keycols[idx];

		std::string foreignkcolname = GetName() + "_" + pkeycol->GetName();
		//If the parent table has a compound key, each part of the key will be
		//part of the subtable's compound primary key, as well as part of its compound foreign key
		subtable->AddColumn(const_cast<const std::string&>(foreignkcolname),
				pkeycol->GetType(),
				SQLiteColumn::KeyType::KEY_PRIMARY_AND_FOREIGN, this, pkeycol->GetName());
	}

	subtable->AddColumn("subtable_index", SQLiteVariant::StoredType::VARINT,
			SQLiteColumn::KeyType::KEY_PRIMARY, this);

	return subtable;
}

SQLiteTable* SQLiteTable::GetSubTable(const std::string& name)
{
	return m_subtablemap[GetName() + "_" + name];
}

SQLiteTable::SQLiteTable(sqlite3* pDB, const char* tablename)
{
#ifndef TESTING_
	m_refcount = 1;
#endif
	m_pDB = pDB;
	m_tablename = tablename;
	m_bIsSubTable = false;
}

SQLiteTable::~SQLiteTable()
{
	dbgprintf("Destroying table %s\n", GetName().c_str());
	size_t idx = 0, len = m_columns.size();
	for(; idx < len; ++idx)
	{
		if(m_columns[idx])
			delete m_columns[idx];
	}

	for(std::map<const std::string, SQLiteTable*>::iterator it = m_subtablemap.begin();
	    it != m_subtablemap.end(); ++it)
	{
		delete it->second;
	}
}

#ifndef TESTING_
void SQLiteTable::AddRef()
{
	asAtomicInc(m_refcount);
}

void SQLiteTable::Release()
{
	asAtomicDec(m_refcount);
	if(!m_refcount)
	{
		delete this;
	}
}

SQLiteTable* SQLiteTable::Factory(const std::string& tablename)
{
	return new SQLiteTable(SQLiteTable::GetDBConnection(), tablename.c_str());
}

#endif

bool SQLiteTable::AddColumn(const std::string& name, SQLiteVariant::StoredType vartype,
			SQLiteColumn::KeyType keytype, SQLiteTable* foreigntable,
			const std::string& foreignname)
{
	SQLiteColumn* newcol = new SQLiteColumn(name, vartype, keytype, foreigntable, foreignname);
	if(!newcol)
	{
		return false;
	}

	switch(keytype)
	{
	case SQLiteColumn::KeyType::KEY_NONE:
		break;
	case SQLiteColumn::KeyType::KEY_AUTO_PRIMARY:
		//An automatic key in sqlite is just an INTEGER PRIMARY KEY, though it will reuse previously used ROWIDs.
		//Autoincrementing keys will not reuse previously used ROWIDs.
		newcol->SetColumnType(SQLiteVariant::StoredType::VARINT);
		//We must drop through to the next case!
	case SQLiteColumn::KeyType::KEY_PRIMARY:
		m_primary_keycols.push_back(newcol);
		break;
	case SQLiteColumn::KeyType::KEY_FOREIGN:
		if(!foreigntable || foreignname.empty())
		{
			delete newcol;
			return false;
		}
		m_foreign_keycols.push_back(newcol);
		break;
	case SQLiteColumn::KeyType::KEY_PRIMARY_AND_FOREIGN:
		if(!foreigntable || foreignname.empty())
		{
			delete newcol;
			return false;
		}
		m_foreign_keycols.push_back(newcol);
		m_primary_keycols.push_back(newcol);
	default:
		break;
	}
	m_columns.push_back(newcol);
	return true;
}

std::string SQLiteTable::ProduceUpdateList()
{
	std::string buffer;
	buffer.reserve(128);
	size_t idx = 0, len = m_columns.size();
	for(; idx < len; ++idx)
	{
		if(m_columns[idx]->IsPrimaryKey() || m_columns[idx]->IsForeignKey())
			continue;
		const std::string& propname = m_columns[idx]->GetName();
		buffer += propname + "=$" + propname;
		if(idx < len - 1)
		{
			buffer.append(", ");
		}
	}
	return buffer;
}

std::string SQLiteTable::ProducePlaceholderList()
{
	std::string buffer;
	buffer.reserve(128);
	size_t idx = 0, len = m_columns.size();
	for(; idx < len; ++idx)
	{
		buffer += "$" + m_columns[idx]->GetName();
		if(idx < len - 1)
		{
			buffer.append(", ");
		}
	}
	return buffer;
}

std::string SQLiteTable::ProduceInsertValuesNameList()
{
	std::string buffer;
	buffer.reserve(128);
	size_t idx = 0, len = m_columns.size();
	for(; idx < len; ++idx)
	{
		buffer += m_columns[idx]->GetName();
		if(idx < len - 1)
		{
			buffer.append(", ");
		}
	}
	return buffer;
}

std::string SQLiteTable::ProducePropertyNameList()
{
	std::string buffer;
	buffer.reserve(256);
	size_t idx = 0, len = m_columns.size();

	for(; idx < len; ++idx)
	{
		SQLiteColumn* pcol = m_columns[idx];
		buffer += pcol->GetName() + " " + pcol->GetTypeAsString() + ", ";
	}

	buffer += "PRIMARY KEY(" + GetPrimaryKeyStringList() + ")";


	if(!m_foreign_keycols.empty())
	{
		SQLiteTable* foreigntable = m_foreign_keycols.back()->GetForeignTable();
		if(foreigntable)
		{
			buffer += ", FOREIGN KEY(" + GetForeignKeyStringList() + ") REFERENCES " +
				foreigntable->m_tablename + "("
				+ foreigntable->GetPrimaryKeyStringList() + ")";
		}
	}

	return buffer;
}

std::string SQLiteTable::GetPrimaryKeyStringList()
{
	std::string retval;
	retval.reserve(64);
	size_t idx = 0;
	size_t len = m_primary_keycols.size();
	for(; idx < len; ++idx)
	{
		if(idx < (len - 1))
		{
			retval += m_primary_keycols[idx]->GetName() + ",";
		}
		else
		{
			retval += m_primary_keycols[idx]->GetName();
		}
	}
	return retval;
}

std::string SQLiteTable::GetForeignKeyStringList()
{
	std::string retval;
	retval.reserve(64);
	size_t idx = 0;
	size_t len = m_foreign_keycols.size();
	for(; idx < len; ++idx)
	{
		if(idx < (len -1))
		{
			retval += m_foreign_keycols[idx]->GetName() + "," ;
		}
		else
		{
			retval += m_foreign_keycols[idx]->GetName();
		}
	}
	return retval;
}

std::string SQLiteTable::ProduceSubTableSelectConditionString()
{
	if(m_foreign_keycols.empty())
	{
		dbgprintf("SubTable has no foreign keycols!\n");
	}
	else
	{
		dbgprintf("SubTable has %lu foreign keycols.\n", m_foreign_keycols.size());
	}

	std::string retval;
	retval.reserve(128);
	for(size_t idx = 0, len = m_foreign_keycols.size();
	    idx < len; ++idx)
	{
		SQLiteColumn* pkeycol = m_foreign_keycols[idx];
		dbgprintf("Found keycol with name '%s'\n", pkeycol->GetName().c_str());
		if(idx < (len - 1))
		{
			retval += pkeycol->GetName() + "=$" + pkeycol->GetName() + " AND ";
		}
		else
		{
			retval += pkeycol->GetName() + "=$" + pkeycol->GetName();
		}
	}
	return retval;
}

std::string SQLiteTable::ProduceSelectConditionString()
{
	size_t pkeys = m_primary_keycols.size();
	if(1 == pkeys)
	{
		SQLiteColumn* pkeycol = m_primary_keycols.back();
		return pkeycol->GetName() + "=$" + pkeycol->GetName();
	}
	else if(pkeys > 1)
	{
		size_t idx = 0;
		std::string retval;
		retval.reserve(128);
		for(; idx < pkeys; ++idx)
		{
			SQLiteColumn* pkeycol = m_primary_keycols[idx];
			if(idx < (pkeys - 1))
			{
				retval += pkeycol->GetName() + "=$" + pkeycol->GetName() + " AND ";
			}
			else
			{
				retval += pkeycol->GetName() + "=$" + pkeycol->GetName();
			}
		}
		return retval;
	}
	else
	{
		return "";
	}
}

#ifndef TESTING_
bool SQLiteTable::LoadSubTable(SQLiteRow* parent_row, CScriptArray* resultarray)
{
/*	asIScriptContext* ctx = asGetActiveContext();
	if(!m_bIsSubTable)
	{
	ctx->SetException("Table is not a subtable.");
	//return SQLITE_ERROR;
	return false;
	}

	int result = DoesSQLiteTableExist(m_pDB, m_tablename.c_str());
	if(result <= 0)
	{
	ctx->SetException("Table does not exist.");
	//return SQLITE_ERROR;
	return false;
	}

	sqlite3_stmt* query = 0;
	std::string selectquerystr = "select " + ProduceInsertValuesNameList() + " from " +
	m_tablename + " where " + ProduceSelectConditionString();

	dbgprintf("Query: %s\n", selectquerystr.c_str());
	result = sqlite3_prepare_v2(m_pDB, selectquerystr.c_str(), -1, &query, 0);
	if(SQLITE_OK != result)
	{
	dbgprintf("LoadRow prepare statement failure.\n");
	//return result;
	return false;
	}

	size_t idx = 1;

	if(SQLITE_OK != BindVariantToStatement(query,
	parent_row->GetColumnValue(m_subtableforeignkey->GetName()), 1))
	{
	dbgprintf("Failed to bind key value to sqlite statement.\n");
	sqlite3_finalize(query);
	return false;
	}

	SQLiteRow* pRow = 0;
	do
	{
	result = sqlite3_step(query);
	if(SQLITE_ROW != result)
	{
	break;
	}

	pRow = CreateRow();
	int columns = sqlite3_column_count(query);
	int idx = 0;
	for(; idx < columns; ++idx)
	{
	switch(sqlite3_column_type(query, idx))
	{
	case SQLITE_INTEGER:
	pRow->SetColumnValue(m_columns[idx]->GetName(), sqlite3_column_int(query, idx));
	break;
	case SQLITE_FLOAT:
	pRow->SetColumnValue(m_columns[idx]->GetName(), sqlite3_column_double(query, idx));
	break;
	case SQLITE_BLOB:
	pRow->SetColumnValue(m_columns[idx]->GetName(),
	(const char*) sqlite3_column_blob(query, idx), sqlite3_column_bytes(query, idx));
	break;
	case SQLITE_NULL:
	break;
	case SQLITE3_TEXT:
	{
	std::string tmp(reinterpret_cast<const char*>(sqlite3_column_text(query, idx)));
	pRow->SetColumnValue(m_columns[idx]->GetName(), tmp);
	}
	break;
	default:
	dbgprintf("Unknown?\n");
	break;
	}
	}
	//pRow->AddRef();
	resultarray->InsertLast(pRow);

	} while(SQLITE_ROW == result);

	sqlite3_finalize(query);
*/
	return true;
}
#else
bool SQLiteTable::LoadSubTable(SQLiteRow* parent_row, std::vector<SQLiteRow*>& resultarray)
{
	//Called from the sub table

	//asIScriptContext* ctx = asGetActiveContext();
	if(!m_bIsSubTable)
	{
		//ctx->SetException("Table is not a subtable.");
		//return SQLITE_ERROR;
		return false;
	}

	int result = DoesSQLiteTableExist(m_pDB, m_tablename.c_str());
	if(result <= 0)
	{
		//ctx->SetException("Table does not exist.");
		//return SQLITE_ERROR;
		return false;
	}

	sqlite3_stmt* query = 0;
	std::string selectquerystr = "select " + ProduceInsertValuesNameList() + " from " +
		m_tablename + " where " + ProduceSubTableSelectConditionString() + ";";

	dbgprintf("LoadSubTable Query: %s\n", selectquerystr.c_str());
	result = sqlite3_prepare_v2(m_pDB, selectquerystr.c_str(), -1, &query, 0);
	if(SQLITE_OK != result)
	{
		dbgprintf("LoadRow prepare statement failure.\n");
		//return result;
		return false;
	}

	//We want all rows from this table which have a foreign key matching the
	//primary key of our parent table. The primary key of a SubTable (array) is an index.

	for(size_t idx = 0, len = m_foreign_keycols.size();
	    idx < len; ++idx)
	{
		SQLiteColumn* fcol = m_foreign_keycols[idx];
		if(SQLITE_OK != BindVariantToStatement(query,
							parent_row->GetColumnValue(fcol->GetForeignName()),
							idx + 1))
		{
			dbgprintf("Sub Table: Failed to bind key value to column %s\n",
				fcol->GetForeignName().c_str());
			sqlite3_finalize(query);
			return false;
		}

	}

	SQLiteRow* pRow = 0;
	do
	{
		result = sqlite3_step(query);
		if(SQLITE_ROW != result)
		{
			break;
		}

		resultarray.emplace_back(new SQLiteRow(this));
		pRow = resultarray.back();
		int columns = sqlite3_column_count(query);
		int idx = 0;
		for(; idx < columns; ++idx)
		{
			switch(sqlite3_column_type(query, idx))
			{
			case SQLITE_INTEGER:
				pRow->SetColumnValue(m_columns[idx]->GetName(), sqlite3_column_int(query, idx));
				break;
			case SQLITE_FLOAT:
				pRow->SetColumnValue(m_columns[idx]->GetName(), sqlite3_column_double(query, idx));
				break;
			case SQLITE_BLOB:
				pRow->SetColumnValue(m_columns[idx]->GetName(),
						(const char*) sqlite3_column_blob(query, idx), sqlite3_column_bytes(query, idx));
				break;
			case SQLITE_NULL:
				break;
			case SQLITE3_TEXT:
			{
				std::string tmp(reinterpret_cast<const char*>(sqlite3_column_text(query, idx)));
				pRow->SetColumnValue(m_columns[idx]->GetName(), tmp);
			}
			break;
			default:
				dbgprintf("SubTable: Unknown value type?\n");
				return false;
				break;
			}
		}
		//pRow->AddRef();
		//resultarray->InsertLast(pRow);

	} while(SQLITE_ROW == result);

	sqlite3_finalize(query);
	return true;

}
#endif

int SQLiteTable::LoadRow(SQLiteRow* pRow)
{
	if(!pRow)
	{
		return SQLITE_ERROR;
	}
	if(!m_primary_keycols.size())
	{
		return SQLITE_ERROR;
	}

	int result = DoesSQLiteTableExist(m_pDB, m_tablename.c_str());

	if(result <= 0)
	{
		return SQLITE_ERROR;
	}

	sqlite3_stmt* query = 0;
	std::string selectquerystr = "select " + ProduceInsertValuesNameList() + " from " +
		m_tablename + " where " + ProduceSelectConditionString() + ";";
	dbgprintf("Query: %s\n", selectquerystr.c_str());
	result = sqlite3_prepare_v2(m_pDB, selectquerystr.c_str(), -1, &query, 0);
	if(SQLITE_OK != result)
	{
		dbgprintf("LoadRow prepare statement failure.\n");
		return result;
	}

	for(size_t idx = 0, len = m_primary_keycols.size();
	    idx < len; ++idx)
	{
		SQLiteColumn* col = m_primary_keycols[idx];
		if(SQLITE_OK != BindVariantToStatement(query,
							pRow->GetColumnValue(col->GetName()), idx + 1))
		{
			dbgprintf("Failed to bind key value to sqlite statement at position %lu.\n", idx + 1);
			dbgprintf("Tried to get value for column '%s'\n", col->GetName().c_str());
			dbgprintf("Load query was: %s\n", selectquerystr.c_str());
			sqlite3_finalize(query);
			return SQLITE_ERROR;
		}
	}

	result = sqlite3_step(query);
	if(SQLITE_ROW != result)
	{
		dbgprintf("sqlite step had no result: %d\n", result);
		sqlite3_finalize(query);
		return result;
	}

	int columns = sqlite3_column_count(query);
	int idx = 0;
	for(; idx < columns; ++idx)
	{
		switch(sqlite3_column_type(query, idx))
		{
		case SQLITE_INTEGER:
			pRow->SetColumnValue(m_columns[idx]->GetName(), sqlite3_column_int(query, idx));
			break;
		case SQLITE_FLOAT:
			pRow->SetColumnValue(m_columns[idx]->GetName(), sqlite3_column_double(query, idx));
			break;
		case SQLITE_BLOB:
			pRow->SetColumnValue(m_columns[idx]->GetName(),
					(const char*) sqlite3_column_blob(query, idx), sqlite3_column_bytes(query, idx));
			break;
		case SQLITE_NULL:
			break;
		case SQLITE3_TEXT:
		{
			std::string tmp(reinterpret_cast<const char*>(sqlite3_column_text(query, idx)));
			pRow->SetColumnValue(m_columns[idx]->GetName(), tmp);
		}
		break;
		default:
			dbgprintf("Unknown?\n");
			break;
		}
	}

	sqlite3_finalize(query);

	return SQLITE_OK;
}

int SQLiteTable::StoreRow(SQLiteRow* pRow, SQLiteRow* pParentRow)
{
	if(!m_primary_keycols.size())
	{
		dbgprintf("No primary key.\n");
		return SQLITE_ERROR;
	}

	int result = DoesSQLiteTableExist(m_pDB, m_tablename.c_str());
	if(0 == result)
	{
		//Create table if it does not already exist
		std::string querystr = "CREATE TABLE " + m_tablename + "(" + ProducePropertyNameList() + ");";
		if(SQLITE_DONE != ExecSQLiteStatement(m_pDB, querystr.c_str()))
		{
			dbgprintf("Failed to create table. Query was: %s\nError was: %s\n", querystr.c_str(),
				sqlite3_errmsg(m_pDB));
			return SQLITE_ERROR;
		}
		querystr = "CREATE UNIQUE INDEX idx_" + m_tablename + " ON " + m_tablename
			+ " (" + GetPrimaryKeyStringList()
			+ ");";
		if(SQLITE_DONE != ExecSQLiteStatement(m_pDB, querystr.c_str()))
		{
			dbgprintf("Failed to create table index.\n");
			return SQLITE_ERROR;
		}
	}
	else if(result < 0)
	{
		dbgprintf("Error %d executing sql statement to check table '%s' existence.\n",
			result, m_tablename.c_str());
		return SQLITE_ERROR;
	}

	std::set<std::string> columnset;
	if(SQLITE_OK != GetTableColumns(m_pDB, m_tablename.c_str(), columnset))
	{
		return SQLITE_ERROR;
	}

	for(SQLiteColumn* pcol : m_columns)
	{
		if(columnset.end() == columnset.find(pcol->GetName()))
		{
			dbgprintf("Table does not contain column %s\n", pcol->GetName().c_str());
			if(SQLITE_DONE == AddColumnToSQLiteTable(m_pDB, m_tablename.c_str(),
									pcol->GetName().c_str(),
									pcol->GetTypeAsString().c_str()))
			{
				dbgprintf("Added column %s successfully\n", pcol->GetName().c_str());
			}
			else
			{
				dbgprintf("Failed to add column.\n");
				return SQLITE_ERROR;
			}
		}
	}

	return PerformUpsert(pRow, pParentRow);
}

SQLiteRow* SQLiteTable::CreateRow()
{
	return new SQLiteRow(this);
}

int SQLiteTable::PerformUpsert(SQLiteRow* pRow, SQLiteRow* pParentRow)
{
	std::string insertstr;
	insertstr.reserve(256);
	insertstr = "INSERT INTO " + m_tablename;
	insertstr += "(" + ProduceInsertValuesNameList() + ") VALUES(";
	insertstr += ProducePlaceholderList() + ") ON CONFLICT(";
	insertstr += GetPrimaryKeyStringList() +") DO UPDATE SET ";
	insertstr += ProduceUpdateList() + ";";

	dbgprintf("Insert str: %s\n", insertstr.c_str());
	sqlite3_stmt* query = 0;
	if(SQLITE_OK != sqlite3_prepare_v2(m_pDB, insertstr.c_str(), insertstr.length(),  &query, 0))
	{
		dbgprintf("Failed to prepare insert statement\n");
		return SQLITE_ERROR;
	}

	if(SQLITE_OK != sqlite3_bind_text(query, 1, m_tablename.c_str(), m_tablename.length(), 0))
	{
		dbgprintf("Failed to bind tablename\n");
		sqlite3_finalize(query);
		return SQLITE_ERROR;
	}

	for(size_t idx = 0, len = m_columns.size(); idx < len; ++idx)
	{
		SQLiteColumn* pcol = m_columns[idx];
		SQLiteVariant* var = pRow->GetColumnValue(pcol->GetName());

		if(pcol->IsForeignKey() && !var)
		{
			SQLiteTable* foreigntable = pcol->GetForeignTable();
			if(foreigntable && pParentRow)
			{
				std::string foreignkeyname = pcol->GetForeignName();
				std::string parentcolvalue;
				pParentRow->GetColumnValue(foreignkeyname, parentcolvalue);
				dbgprintf("Table %s: Setting column value for foreign keycol '%s' ('%s' locally) to %s\n",
					GetName().c_str(), foreignkeyname.c_str(), pcol->GetName().c_str(),
					parentcolvalue.c_str());
				pRow->SetColumnValue(pcol->GetName(), parentcolvalue);
				var = pRow->GetColumnValue(pcol->GetName());
			}
			else
			{
				dbgprintf("Table has a foreign key constraint"
					" but was not given a foreign table.\n");

				sqlite3_finalize(query);
				return SQLITE_ERROR;
			}
		}

		if(SQLITE_OK != BindVariantToStatement(query, var, idx + 1))
		{
			sqlite3_finalize(query);
			dbgprintf("Failed to bind value %lu for column %s\n", idx, pcol->GetName().c_str());
			return SQLITE_ERROR;
		}
	}

	if(SQLITE_DONE != sqlite3_step(query))
	{
		sqlite3_finalize(query);
		return SQLITE_ERROR;
	}
	sqlite3_finalize(query);
	return SQLITE_OK;
}
