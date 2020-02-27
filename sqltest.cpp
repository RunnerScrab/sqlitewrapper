#include "sqlitetable.h"
#include "sqliterow.h"

#include <cstdio>

int main(void)
{
	sqlite3* pDB = 0;
	if(SQLITE_OK != sqlite3_open("./testdb.db", &pDB))
	{
		printf("Error opening database.\n");
		return 0;
	}
	if(SQLITE_OK != sqlite3_exec(pDB, "PRAGMA foreign_keys = ON;", 0, 0, 0))
	{
		printf("Failure executing pragma\n");
		return 0;
	}

	SQLiteTable testtable(pDB, "testtable");
	testtable.AddColumn("uuid",  SQLiteVariant::StoredType::VARTEXT, SQLiteColumn::KeyType::KEY_PRIMARY);
	testtable.AddColumn("age", SQLiteVariant::StoredType::VARINT);
	testtable.AddColumn("sdesc", SQLiteVariant::StoredType::VARTEXT);
	testtable.AddColumn("height", SQLiteVariant::StoredType::VARREAL);
	testtable.AddColumn("mass", SQLiteVariant::StoredType::VARREAL);

	SQLiteTable inventory(pDB, "inventorytable", &testtable);
	inventory.AddColumn("id", SQLiteVariant::StoredType::VARINT, SQLiteColumn::KeyType::KEY_PRIMARY);
	inventory.AddColumn("owner_uuid", SQLiteVariant::StoredType::VARTEXT, SQLiteColumn::KeyType::KEY_FOREIGN);
	inventory.AddColumn("item_name", SQLiteVariant::StoredType::VARTEXT);
	inventory.AddColumn("item_sdesc", SQLiteVariant::StoredType::VARTEXT);

	SQLiteRow row(&testtable);
	row.SetColumnValue("uuid", "meower1");
	row.SetColumnValue("age", 34);
	row.SetColumnValue("sdesc", "a meower");
	row.SetColumnValue("height", 20);

	if(SQLITE_ERROR == testtable.StoreRow(&row))
	{
		printf("FAILED TO STORE TESTTABLE ROW\n");
	}

	SQLiteRow invrow(&inventory);
	invrow.SetColumnValue("id", 1);
	//invrow.SetColumnValue("owner_uuid", "meower1");
	invrow.SetColumnValue("item_name", "a thingy");

	if(SQLITE_ERROR == inventory.StoreRow(&invrow, &row))
	{
		printf("FAILED TO STORE INVENTORY ROW\n");
	}

	SQLiteRow searchrow(&testtable);
	searchrow.SetColumnValue("uuid", "meower1");
	if(SQLITE_OK == testtable.LoadRow(&searchrow))
	{
		std::string uuid, sdesc;
		int age = 0;
		searchrow.GetColumnValue("uuid", uuid);
		searchrow.GetColumnValue("sdesc", sdesc);
		searchrow.GetColumnValue("age", age);
		printf("Found %s of age %d and sdesc %s\n",
			uuid.c_str(), age, sdesc.c_str());
	}

	sqlite3_close(pDB);

	return 0;
}
