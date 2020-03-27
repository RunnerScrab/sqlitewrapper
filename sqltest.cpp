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

	SQLiteTable* testinvtable = testtable.CreateSubTable("testinv");
	testinvtable->AddColumn("name", SQLiteVariant::StoredType::VARTEXT);
	testinvtable->AddColumn("sdesc", SQLiteVariant::StoredType::VARTEXT);

	SQLiteTable inventory(pDB, "inventorytable");
	inventory.AddColumn("id", SQLiteVariant::StoredType::VARINT, SQLiteColumn::KeyType::KEY_PRIMARY);
	inventory.AddColumn("owner_uuid", SQLiteVariant::StoredType::VARTEXT,
			SQLiteColumn::KeyType::KEY_PRIMARY_AND_FOREIGN,
			&testtable, "uuid");
	inventory.AddColumn("item_name", SQLiteVariant::StoredType::VARTEXT);
	inventory.AddColumn("item_sdesc", SQLiteVariant::StoredType::VARTEXT);

	SQLiteRow row(&testtable);
	row.SetColumnValue("uuid", "meower1");
	row.SetColumnValue("age", 34);
	row.SetColumnValue("sdesc", "a meower");
	row.SetColumnValue("height", 20);

	printf("Attempting to store testtable row.\n");
	if(SQLITE_ERROR == testtable.StoreRow(&row))
	{
		printf("FAILED TO STORE TESTTABLE ROW\n");
	}
	else
	{
		printf("Store reported SUCCESS\n");
	}

	SQLiteRow* trow = testinvtable->CreateRow();

	printf("Attempting to store subtable.\n");
	for(size_t idx = 0; idx < 10; ++idx)
	{
		trow->SetColumnValue("subtable_index", static_cast<unsigned int>(idx));
		trow->SetColumnValue("name", "meower");
		trow->SetColumnValue("sdesc", "a meower");
		trow->StoreChildRowIntoDB(&row);
	}
	delete trow;

	SQLiteRow invrow(&inventory);
	invrow.SetColumnValue("id", 2);
	invrow.SetColumnValue("owner_uuid", "meower1");
	invrow.SetColumnValue("item_name", "another thingy");
	invrow.SetColumnValue("item_sdesc", "this is another thingy");

	printf("Attempting to store inventory row.\n");
	if(SQLITE_ERROR == inventory.StoreRow(&invrow, &row))
	{
		printf("FAILED TO STORE INVENTORY ROW\n");
	}
	else
	{
		printf("Store reported SUCCESS\n");
	}

	SQLiteRow searchrow(&testtable);
	searchrow.SetColumnValue("uuid", "meower1");
	printf("Attempting to load testtable row.\n");
	if(SQLITE_OK == testtable.LoadRow(&searchrow))
	{
		printf("Load SUCCESS\n");
		std::string uuid, sdesc;
		int age = 0;
		searchrow.GetColumnValue("uuid", uuid);
		searchrow.GetColumnValue("sdesc", sdesc);
		searchrow.GetColumnValue("age", age);
		printf("Found %s of age %d and sdesc %s\n",
			uuid.c_str(), age, sdesc.c_str());
	}
	else
	{
		printf("Load FAILURE\n");
	}


	searchrow.ClearValues();
	searchrow.SetColumnValue("owner_uuid", "meower1");
	searchrow.SetColumnValue("id", 2);
	printf("Attempting to load inventory row.\n");
	if(SQLITE_OK == inventory.LoadRow(&searchrow))
	{
		printf("Load SUCCESS\n");
		std::string sdesc, uuid;
		searchrow.GetColumnValue("item_name", uuid);
		searchrow.GetColumnValue("item_sdesc", sdesc);
		printf("Found %s with sdesc %s\n", uuid.c_str(), sdesc.c_str());
	}
	else
	{
		printf("Load FAILURE\n");
	}



	std::vector<SQLiteRow*> outarray;
	if(testinvtable->LoadSubTable(&row, outarray))
	{
		printf("LoadSubTable succeeded. There are %lu elements in the array.\n",
			outarray.size());
	}
	else
	{
		printf("LoadSubTable FAILED!\n");
	}

	sqlite3_close(pDB);
	printf("Closed database connection.\n");
	return 0;
}
