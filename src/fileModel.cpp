#include <fileModel.h>

FileDataModel::FileDataModel(HashDb& db, QObject* _parent): QAbstractItemModel(_parent), database(db)
	{
	//TODO: How to connect to the SQL storage????
	}
int FileDataModel::columnCount(const QModelIndex &parent) const
	{
	// Always static
	return FileDataModel::MAX_COLUMNS;
	}
int FileDataModel::rowCount(const QModelIndex &parent) const
	{
	//TODO: Lookup the number of entires from SQL storage
	return 0;
	}

QVariant FileDataModel::data(const QModelIndex &index, int role) const
	{
	//TODO: Lookup the index in the SQL storage where all the data is stored.
	return QVariant();
	}
QModelIndex FileDataModel::index(int row, int column, const QModelIndex& parent) const
	{
	return QModelIndex();
	}
QModelIndex FileDataModel::parent(const QModelIndex& index) const
	{
	return QModelIndex();
	}
